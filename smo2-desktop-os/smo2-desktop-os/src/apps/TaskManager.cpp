// =============================================================================
// CSOPESY SMO2 - Task Manager (Stage 3, Member 3: Raina)
// See TaskManager.h for the overview and config.ini [taskmgr] for all keys.
// =============================================================================
#include "apps/TaskManager.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "imgui.h"
#include "core/AppState.h"

namespace TaskManager {
namespace {

// ------------------------------------------------------------------ Settings
// Every value a quiz question could vary is read from config.ini.
struct Settings {
    int   processCount = 14;
    float cpuMin = 0.0f, cpuMax = 8.0f;         // percent, per process
    float memMin = 20.0f, memMax = 600.0f;       // MB, per process
    int   refreshMs = 1000;                     // 0 = frozen values
    float fluctuation = 0.15f;                  // 0..1, fraction of value
    int   seed = 0;                             // 0 = different every run
    float totalMemMb = 16384.0f;
    std::string cpuName;
    float cpuGhz = 3.20f;
    int   cores = 4, logical = 8;
    int   history = 60;                         // graph samples
    int   suspended = 1;                        // suspended processes
    bool  showDiskNet = true;
    bool  heatmap = true;
    std::string defaultTab = "processes";       // processes | performance
    std::vector<std::string> names;             // optional custom names
    // Colors (config: #RRGGBB, #RRGGBBAA or r,g,b[,a])
    ImVec4 cpuColor, memColor, diskColor, netColor, dimColor, heatLow, heatHigh;
};

std::vector<std::string> SplitCsv(const std::string& s) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
        const auto a = item.find_first_not_of(" \t\"");
        const auto b = item.find_last_not_of(" \t\"");
        if (a != std::string::npos) out.push_back(item.substr(a, b - a + 1));
    }
    return out;
}

// "8.00 GB" -> 8192, "512 MB" -> 512. Falls back to 8 GB.
float ParseRamMb(const std::string& text) {
    try {
        size_t used = 0;
        const float n = std::stof(text, &used);
        std::string unit = text.substr(used);
        for (auto& c : unit) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (unit.find("MB") != std::string::npos) return n;
        if (unit.find("TB") != std::string::npos) return n * 1024.0f * 1024.0f;
        return n * 1024.0f;                                   // GB or no unit
    } catch (...) { return 8192.0f; }
}

// "Virtual CPU @ 3.20 GHz" -> 3.20
float ParseGhz(const std::string& text, float def) {
    const size_t at = text.find('@');
    if (at == std::string::npos) return def;
    try { return std::stof(text.substr(at + 1)); } catch (...) { return def; }
}

Settings LoadSettings(const Config& cfg) {
    Settings s;
    s.processCount = cfg.getInt("taskmgr.process_count", 14);
    s.cpuMin       = cfg.getFloat("taskmgr.cpu_min", 0.0f);
    s.cpuMax       = cfg.getFloat("taskmgr.cpu_max", 8.0f);
    s.memMin       = cfg.getFloat("taskmgr.mem_min_mb", 20.0f);
    s.memMax       = cfg.getFloat("taskmgr.mem_max_mb", 600.0f);
    s.refreshMs    = cfg.getInt("taskmgr.refresh_ms", 1000);
    s.fluctuation  = cfg.getFloat("taskmgr.fluctuation", 0.15f);
    s.seed         = cfg.getInt("taskmgr.seed", 0);
    // System info defaults come from [about] so Settings and Task Manager agree.
    s.totalMemMb   = cfg.getFloat("taskmgr.total_memory_mb", ParseRamMb(cfg.getString("about.ram", "8.00 GB")));
    s.cpuName      = cfg.getString("taskmgr.cpu_name", cfg.getString("about.processor", "Virtual CPU @ 3.20 GHz"));
    s.cpuGhz       = cfg.getFloat("taskmgr.cpu_ghz", ParseGhz(s.cpuName, 3.20f));
    s.cores        = cfg.getInt("taskmgr.cores", 4);
    s.logical      = cfg.getInt("taskmgr.logical_processors", 8);
    s.history      = cfg.getInt("taskmgr.history_length", 60);
    s.suspended    = cfg.getInt("taskmgr.suspended_count", 1);
    s.showDiskNet  = cfg.getBool("taskmgr.show_disk_network", true);
    s.heatmap      = cfg.getBool("taskmgr.heatmap", true);
    s.defaultTab   = cfg.getChoice("taskmgr.default_tab", "processes");
    s.names        = SplitCsv(cfg.getString("taskmgr.process_names", ""));
    auto color = [&](const char* key, ImU32 def) { return ImGui::ColorConvertU32ToFloat4(cfg.getColor(key, def)); };
    s.cpuColor  = color("taskmgr.cpu_color",    IM_COL32( 97, 173, 255, 255));
    s.memColor  = color("taskmgr.memory_color", IM_COL32(191, 115, 242, 255));
    s.diskColor = color("taskmgr.disk_color",   IM_COL32(115, 217, 115, 255));
    s.netColor  = color("taskmgr.network_color",IM_COL32(242, 179,  77, 255));
    s.dimColor  = color("taskmgr.label_color",  IM_COL32(166, 166, 179, 255));
    s.heatLow   = color("taskmgr.heat_low",     IM_COL32(255, 217,  64, 255));
    s.heatHigh  = color("taskmgr.heat_high",    IM_COL32(255, 115,  26, 255));

    // Sanitize so odd quiz values never crash or look broken.
    s.processCount = std::clamp(s.processCount, 1, 500);
    s.cpuMin = std::clamp(s.cpuMin, 0.0f, 100.0f);
    s.cpuMax = std::clamp(s.cpuMax, 0.0f, 100.0f);
    if (s.cpuMin > s.cpuMax) std::swap(s.cpuMin, s.cpuMax);
    s.memMin = std::max(0.0f, s.memMin);
    s.memMax = std::max(0.0f, s.memMax);
    if (s.memMin > s.memMax) std::swap(s.memMin, s.memMax);
    s.refreshMs   = std::max(0, s.refreshMs);
    s.fluctuation = std::clamp(s.fluctuation, 0.0f, 1.0f);
    s.totalMemMb  = std::max(1.0f, s.totalMemMb);
    s.history     = std::clamp(s.history, 2, 1000);
    s.suspended   = std::clamp(s.suspended, 0, s.processCount);
    s.cores       = std::max(1, s.cores);
    s.logical     = std::max(s.cores, s.logical);
    return s;
}

// ------------------------------------------------------------------ Data
struct Process {
    std::string name;
    int   pid = 0;
    bool  suspended = false;
    float cpuBase = 0, cpu = 0;     // percent
    float memBase = 0, mem = 0;     // MB
    float disk = 0;                 // MB/s
    float net = 0;                  // Mbps
};

enum Column { ColName, ColPid, ColStatus, ColCpu, ColMem, ColDisk, ColNet };

const char* kDefaultNames[] = {
    "CSOPESY Desktop Shell", "File Explorer", "Settings", "Task Manager",
    "Terminal", "Compositor Service", "Desktop Window Manager",
    "Clock Service", "Wallpaper Renderer", "Antimalware Service Executable",
    "Audio Device Graph Isolation", "Network Manager", "Print Spooler",
    "Search Indexer", "Service Host: Local System", "Client Server Runtime",
    "Font Cache Service", "Update Orchestrator", "Registry", "System",
};
constexpr int kDefaultNameCount = sizeof(kDefaultNames) / sizeof(kDefaultNames[0]);

struct TMState {
    bool initialized = false;
    Settings s;
    std::vector<Process> procs;
    std::vector<int> order;                 // sorted view into procs
    std::mt19937 rng;
    double lastRefresh = 0.0, startTime = 0.0;
    std::vector<float> cpuHist, memHist;
    int histOffset = 0;
    int selectedPid = -1;
    int perfSelection = 0;                  // 0 = CPU, 1 = Memory
    bool needsSort = true;
    int sortColumn = ColCpu;                // remembered so refreshes re-sort
    bool sortAscending = false;
    bool firstFrame = true;
    int lastDrawnFrame = -10;               // detects close -> reopen
    float load = 1.0f;                      // system-wide load swing shared by all processes
};

TMState g;

float Rand(float a, float b) {
    if (b <= a) return a;
    return std::uniform_real_distribution<float>(a, b)(g.rng);
}

float TotalCpu() {
    float t = 0; for (auto& p : g.procs) t += p.cpu;
    return std::min(100.0f, t);
}
float TotalMemMb() {
    float t = 0; for (auto& p : g.procs) t += p.mem;
    return t;
}
float MemPercent() { return std::min(100.0f, TotalMemMb() / g.s.totalMemMb * 100.0f); }
float TotalDisk()  { float t = 0; for (auto& p : g.procs) t += p.disk; return t; }
float TotalNet()   { float t = 0; for (auto& p : g.procs) t += p.net;  return t; }

void PushHistory() {
    g.cpuHist[g.histOffset] = TotalCpu();
    g.memHist[g.histOffset] = MemPercent();
    g.histOffset = (g.histOffset + 1) % g.s.history;
}

void Generate(const Config& cfg) {
    g.s = LoadSettings(cfg);
    g.rng.seed(g.s.seed != 0 ? static_cast<unsigned>(g.s.seed) : std::random_device{}());
    g.procs.clear();

    for (int i = 0; i < g.s.processCount; ++i) {
        Process p;
        if (!g.s.names.empty()) {
            p.name = g.s.names[i % g.s.names.size()];
            if (i >= static_cast<int>(g.s.names.size()))
                p.name += " (" + std::to_string(i / g.s.names.size() + 1) + ")";
        } else {
            p.name = kDefaultNames[i % kDefaultNameCount];
            if (i >= kDefaultNameCount)
                p.name += " (" + std::to_string(i / kDefaultNameCount + 1) + ")";
        }
        p.pid     = 1000 + i * 4 + static_cast<int>(Rand(0, 3.99f));
        p.cpuBase = p.cpu = Rand(g.s.cpuMin, g.s.cpuMax);
        p.memBase = p.mem = Rand(g.s.memMin, g.s.memMax);
        p.disk    = Rand(0.0f, 0.5f);
        p.net     = Rand(0.0f, 0.3f);
        g.procs.push_back(p);
    }
    // Suspend the last N processes (background ones in the default list).
    for (int i = 0; i < g.s.suspended; ++i) {
        Process& p = g.procs[g.procs.size() - 1 - i];
        p.suspended = true;
        p.cpu = p.cpuBase = 0.0f;
        p.disk = p.net = 0.0f;
    }

    g.cpuHist.assign(g.s.history, TotalCpu());
    g.memHist.assign(g.s.history, MemPercent());
    g.histOffset = 0;
    g.startTime = g.lastRefresh = ImGui::GetTime();
    g.selectedPid = -1;
    g.perfSelection = 0;
    g.load = 1.0f;
    g.needsSort = true;
    g.firstFrame = true;
    PushHistory();
    g.initialized = true;
}

// Slight live fluctuation around each process's base value.
void Fluctuate() {
    const float f = g.s.fluctuation;
    // Shared load swing (random walk pulled back toward 1.0) so the total CPU
    // graph rises and falls like a real machine instead of averaging out flat.
    g.load += Rand(-f, f) * 1.2f + (1.0f - g.load) * 0.25f;
    g.load = std::clamp(g.load, 1.0f - f, 1.0f + f);
    for (auto& p : g.procs) {
        if (p.suspended) continue;
        const float cpuTarget = p.cpuBase * g.load * (1.0f + Rand(-f, f)) + Rand(-f, f) * (g.s.cpuMax - g.s.cpuMin) * 0.1f;
        p.cpu = std::clamp(p.cpu + (cpuTarget - p.cpu) * 0.6f, g.s.cpuMin, g.s.cpuMax);
        const float memTarget = p.memBase * (1.0f + Rand(-f, f) * 0.3f);    // memory moves less
        p.mem = std::clamp(p.mem + (memTarget - p.mem) * 0.5f, g.s.memMin, g.s.memMax);
        p.disk = (Rand(0, 1) < 0.1f) ? Rand(0.1f, 3.0f) : Rand(0.0f, 0.2f) * f * 4.0f;
        p.net  = (Rand(0, 1) < 0.1f) ? Rand(0.1f, 2.0f) : Rand(0.0f, 0.1f) * f * 4.0f;
    }
    PushHistory();
    g.needsSort = true;
}

void Update() {
    if (g.s.refreshMs <= 0) return;                 // frozen
    const double now = ImGui::GetTime();
    if ((now - g.lastRefresh) * 1000.0 >= g.s.refreshMs) {
        g.lastRefresh = now;
        Fluctuate();
    }
}

void SortOrder() {
    g.order.resize(g.procs.size());
    for (size_t i = 0; i < g.order.size(); ++i) g.order[i] = static_cast<int>(i);
    const int col = g.sortColumn;
    const bool asc = g.sortAscending;
    std::stable_sort(g.order.begin(), g.order.end(), [&](int ia, int ib) {
        const Process& a = g.procs[ia];
        const Process& b = g.procs[ib];
        int c = 0;
        switch (col) {
            case ColName:   c = a.name.compare(b.name); break;
            case ColPid:    c = (a.pid > b.pid) - (a.pid < b.pid); break;
            case ColStatus: c = static_cast<int>(a.suspended) - static_cast<int>(b.suspended); break;
            case ColCpu:    c = (a.cpu > b.cpu) - (a.cpu < b.cpu); break;
            case ColMem:    c = (a.mem > b.mem) - (a.mem < b.mem); break;
            case ColDisk:   c = (a.disk > b.disk) - (a.disk < b.disk); break;
            case ColNet:    c = (a.net > b.net) - (a.net < b.net); break;
        }
        return asc ? c < 0 : c > 0;
    });
    g.needsSort = false;
}

// ------------------------------------------------------------------ Drawing

void HeatCell(float value, float maxValue) {
    if (!g.s.heatmap || maxValue <= 0.0f) return;
    const float t = std::clamp(value / maxValue, 0.0f, 1.0f);
    // Windows-style: pale heat_low for light usage, strong heat_high for heavy usage.
    const ImVec4& lo = g.s.heatLow;
    const ImVec4& hi = g.s.heatHigh;
    const ImVec4 c(lo.x + (hi.x - lo.x) * t, lo.y + (hi.y - lo.y) * t, lo.z + (hi.z - lo.z) * t,
                   (0.08f + 0.50f * t) * (lo.w + (hi.w - lo.w) * t));
    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(c));
}

void RightAlignedText(const char* text) {
    const float w = ImGui::CalcTextSize(text).x;
    const float avail = ImGui::GetContentRegionAvail().x;
    if (avail > w) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + avail - w);
    ImGui::TextUnformatted(text);
}

void FormatMem(char* buf, size_t n, float mb) {
    if (mb >= 1024.0f) std::snprintf(buf, n, "%.1f GB", mb / 1024.0f);
    else               std::snprintf(buf, n, "%.1f MB", mb);
}

// Summary header: usage figures above the table, like Windows' column headers.
// Each card is label / value / detail so nothing has to share one line.
void DrawSummaryHeader() {
    const int cols = g.s.showDiskNet ? 5 : 3;
    if (!ImGui::BeginTable("##summary", cols, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_BordersInnerV))
        return;
    char value[48], detail[48];
    auto card = [&](const char* label, const char* v, const char* d, ImVec4 color) {
        ImGui::TableNextColumn();
        ImGui::TextColored(g.s.dimColor, "%s", label);
        ImGui::TextColored(color, "%s", v);
        ImGui::TextColored(g.s.dimColor, "%s", d);
    };
    const int running = static_cast<int>(std::count_if(g.procs.begin(), g.procs.end(),
                                                       [](const Process& p) { return !p.suspended; }));
    std::snprintf(value, sizeof value, "%d", static_cast<int>(g.procs.size()));
    std::snprintf(detail, sizeof detail, "%d running", running);
    card("Processes", value, detail, ImVec4(1, 1, 1, 1));

    std::snprintf(value, sizeof value, "%.0f%%", TotalCpu());
    std::snprintf(detail, sizeof detail, "%.2f GHz", g.s.cpuGhz);
    card("CPU", value, detail, g.s.cpuColor);

    std::snprintf(value, sizeof value, "%.0f%%", MemPercent());
    FormatMem(detail, sizeof detail, TotalMemMb());
    card("Memory", value, detail, g.s.memColor);

    if (g.s.showDiskNet) {
        std::snprintf(value, sizeof value, "%.1f", TotalDisk());
        card("Disk", value, "MB/s", g.s.diskColor);
        std::snprintf(value, sizeof value, "%.1f", TotalNet());
        card("Network", value, "Mbps", g.s.netColor);
    }
    ImGui::EndTable();
}

void DrawProcessesTab() {
    DrawSummaryHeader();
    ImGui::Spacing();

    const ImGuiStyle& st = ImGui::GetStyle();
    const float footer = ImGui::GetFrameHeight() + st.ItemSpacing.y;
    const int cols = g.s.showDiskNet ? 7 : 5;

    // Number columns are sized to their widest possible text so values are
    // never cut off. Name takes the remaining space; if the window is too
    // narrow for everything, the table scrolls sideways instead of clipping.
    auto textW = [&](const char* t) { return ImGui::CalcTextSize(t).x + st.CellPadding.x * 2.0f; };
    const float wPid    = textW("00000");
    const float wStatus = textW("Suspended");
    const float wCpu    = textW("100.0%");
    const float wMem    = textW("999.9 MB");
    const float wDisk   = textW("0.0 MB/s");
    const float wNet    = textW("0.0 Mbps");
    const float wNameMin = textW("Antimalware Service");
    float needed = wNameMin + wPid + wStatus + wCpu + wMem;
    if (g.s.showDiskNet) needed += wDisk + wNet;
    needed += cols * 2.0f;
    // Subtract the vertical scrollbar so the last column is never hidden under it.
    const float innerW = std::max(ImGui::GetContentRegionAvail().x - st.ScrollbarSize, needed);

    const ImGuiTableFlags flags = ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX |
        ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_Resizable;

    if (ImGui::BeginTable("##processes", cols, flags, ImVec2(0, -footer), innerW)) {
        ImGui::TableSetupScrollFreeze(1, 1);           // header row + Name column stay visible
        ImGui::TableSetupColumn("Name",   ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoHide, 0.0f, ColName);
        ImGui::TableSetupColumn("PID",    ImGuiTableColumnFlags_WidthFixed, wPid, ColPid);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, wStatus, ColStatus);
        ImGui::TableSetupColumn("CPU",    ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort |
                                          ImGuiTableColumnFlags_PreferSortDescending, wCpu, ColCpu);
        ImGui::TableSetupColumn("Memory", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending, wMem, ColMem);
        if (g.s.showDiskNet) {
            ImGui::TableSetupColumn("Disk",    ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending, wDisk, ColDisk);
            ImGui::TableSetupColumn("Network", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_PreferSortDescending, wNet, ColNet);
        }
        ImGui::TableHeadersRow();

        if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs()) {
            if (specs->SpecsDirty && specs->SpecsCount > 0) {
                g.sortColumn = static_cast<int>(specs->Specs[0].ColumnUserID);
                g.sortAscending = specs->Specs[0].SortDirection == ImGuiSortDirection_Ascending;
                g.needsSort = true;
                specs->SpecsDirty = false;
            }
        }
        if (g.needsSort || g.order.size() != g.procs.size()) SortOrder();

        char buf[48];
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(g.order.size()));
        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                const Process& p = g.procs[g.order[row]];
                ImGui::TableNextRow();
                ImGui::PushID(p.pid);

                ImGui::TableSetColumnIndex(ColName);
                if (ImGui::Selectable(p.name.c_str(), g.selectedPid == p.pid,
                                      ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap))
                    g.selectedPid = (g.selectedPid == p.pid) ? -1 : p.pid;
                if (ImGui::IsItemHovered() && ImGui::CalcTextSize(p.name.c_str()).x > ImGui::GetItemRectSize().x)
                    ImGui::SetTooltip("%s", p.name.c_str());

                ImGui::TableSetColumnIndex(ColPid);
                std::snprintf(buf, sizeof buf, "%d", p.pid);
                RightAlignedText(buf);

                ImGui::TableSetColumnIndex(ColStatus);
                if (p.suspended) ImGui::TextColored(g.s.dimColor, "Suspended");
                else             ImGui::TextUnformatted("Running");

                ImGui::TableSetColumnIndex(ColCpu);
                HeatCell(p.cpu, std::max(g.s.cpuMax, 1.0f));
                std::snprintf(buf, sizeof buf, "%.1f%%", p.cpu);
                RightAlignedText(buf);

                ImGui::TableSetColumnIndex(ColMem);
                HeatCell(p.mem, std::max(g.s.memMax, 1.0f));
                FormatMem(buf, sizeof buf, p.mem);
                RightAlignedText(buf);

                if (g.s.showDiskNet) {
                    ImGui::TableSetColumnIndex(ColDisk);
                    HeatCell(p.disk, 3.0f);
                    std::snprintf(buf, sizeof buf, "%.1f MB/s", p.disk);
                    RightAlignedText(buf);

                    ImGui::TableSetColumnIndex(ColNet);
                    HeatCell(p.net, 2.0f);
                    std::snprintf(buf, sizeof buf, "%.1f Mbps", p.net);
                    RightAlignedText(buf);
                }
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }

    // Footer: End task pinned to the right (like Windows), short status on the left.
    const float btnW = ImGui::CalcTextSize("End task").x + st.FramePadding.x * 2.0f;
    const float rightEdge = ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x;
    ImGui::AlignTextToFramePadding();
    if (g.s.refreshMs > 0) ImGui::TextColored(g.s.dimColor, "Refresh: %d ms", g.s.refreshMs);
    else                   ImGui::TextColored(g.s.dimColor, "Refresh: paused");
    ImGui::SameLine(std::max(ImGui::GetCursorPosX(), rightEdge - btnW));
    ImGui::BeginDisabled(g.selectedPid < 0);
    if (ImGui::Button("End task")) {
        g.procs.erase(std::remove_if(g.procs.begin(), g.procs.end(),
            [](const Process& p) { return p.pid == g.selectedPid; }), g.procs.end());
        g.selectedPid = -1;
        g.needsSort = true;
    }
    ImGui::EndDisabled();
}

// One entry in the Performance side list: sparkline + name + two detail lines.
void DrawPerfListItem(int index, const char* label, const char* line1, const char* line2,
                      const std::vector<float>& hist, ImVec4 color) {
    ImGui::PushID(index);
    const bool selected = g.perfSelection == index;
    const ImVec2 start = ImGui::GetCursorPos();
    const float lineH = ImGui::GetTextLineHeightWithSpacing();
    const float h = lineH * 3.0f + 8.0f;
    if (ImGui::Selectable("##item", selected, 0, ImVec2(0, h))) g.perfSelection = index;
    const float sparkW = std::min(lineH * 2.2f, ImGui::GetContentRegionAvail().x * 0.35f);
    ImGui::SetCursorPos(ImVec2(start.x + 4, start.y + 4));
    ImGui::PushStyleColor(ImGuiCol_PlotLines, color);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0.25f));
    ImGui::PlotLines("##spark", hist.data(), static_cast<int>(hist.size()), g.histOffset,
                     nullptr, 0.0f, 100.0f, ImVec2(sparkW, h - 8));
    ImGui::PopStyleColor(2);
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::TextUnformatted(label);
    ImGui::TextColored(g.s.dimColor, "%s", line1);
    ImGui::TextColored(g.s.dimColor, "%s", line2);
    ImGui::EndGroup();
    // Move below the item and submit a Dummy so ImGui registers the extent.
    ImGui::SetCursorPos(ImVec2(start.x, start.y + h));
    ImGui::Dummy(ImVec2(0, 0));
    ImGui::PopID();
}

// "Label        value" on one line, value right-aligned in its cell.
void StatCell(const char* label, const char* value) {
    ImGui::TableNextColumn();
    ImGui::TextColored(g.s.dimColor, "%s", label);
    ImGui::SameLine();
    RightAlignedText(value);
}

void DrawPerformanceTab() {
    char a[64], b[96];
    const ImGuiStyle& st = ImGui::GetStyle();
    char used[32], total[32];
    FormatMem(used, sizeof used, TotalMemMb());
    FormatMem(total, sizeof total, g.s.totalMemMb);
    std::snprintf(b, sizeof b, "%s / %s", used, total);

    // Side list is as wide as its widest text ("999.9 MB / <total>") plus the
    // sparkline, so it does not change width as values move. Capped at 40%.
    const std::string widest = std::string("999.9 MB / ") + total;
    const float listW = std::min(ImGui::GetContentRegionAvail().x * 0.4f,
        ImGui::GetTextLineHeightWithSpacing() * 2.2f + ImGui::CalcTextSize(widest.c_str()).x +
        st.ItemSpacing.x * 2.0f + st.WindowPadding.x * 2.0f);

    // ---- Side list (CPU / Memory), like Windows' left column
    ImGui::BeginChild("##perf_list", ImVec2(listW, 0));
    char ghz[32];
    std::snprintf(a, sizeof a, "%.0f%%", TotalCpu());
    std::snprintf(ghz, sizeof ghz, "%.2f GHz", g.s.cpuGhz);
    DrawPerfListItem(0, "CPU", a, ghz, g.cpuHist, g.s.cpuColor);
    std::snprintf(a, sizeof a, "%.0f%%", MemPercent());
    DrawPerfListItem(1, "Memory", a, b, g.memHist, g.s.memColor);
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("##perf_detail", ImVec2(0, 0));
    const bool cpu = g.perfSelection == 0;
    const ImVec4 color = cpu ? g.s.cpuColor : g.s.memColor;

    // ---- Header: resource name + hardware name on the right
    ImGui::TextColored(color, "%s", cpu ? "CPU" : "Memory");
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, g.s.dimColor);
    RightAlignedText(cpu ? g.s.cpuName.c_str() : total);
    ImGui::PopStyleColor();
    ImGui::TextColored(g.s.dimColor, "%s", cpu ? "% Utilization" : "Memory usage");

    // ---- Stats are laid out in as many columns as fit, and the graph takes
    //      whatever height is left, so nothing is cut off at any UI scale.
    const float colW = ImGui::CalcTextSize("Logical processors  0:00:00:00").x + st.CellPadding.x * 2.0f + st.ItemSpacing.x * 2.0f;
    const int statCols = std::clamp(static_cast<int>(ImGui::GetContentRegionAvail().x / colW), 1, 4);
    const int statCount = cpu ? 7 : 5;
    const int statRows = (statCount + statCols - 1) / statCols;
    const float statsH = statRows * (ImGui::GetTextLineHeight() + st.CellPadding.y * 2.0f) + st.ItemSpacing.y * 2.0f;
    const float graphH = std::max(ImGui::GetContentRegionAvail().y * 0.4f, ImGui::GetContentRegionAvail().y - statsH);

    std::snprintf(a, sizeof a, "%.0f%%", cpu ? TotalCpu() : MemPercent());
    ImGui::PushStyleColor(ImGuiCol_PlotLines, color);
    ImGui::PlotLines("##graph", cpu ? g.cpuHist.data() : g.memHist.data(), g.s.history,
                     g.histOffset, a, 0.0f, 100.0f, ImVec2(-1, graphH));
    ImGui::PopStyleColor();

    const int upS = static_cast<int>(ImGui::GetTime() - g.startTime);
    if (ImGui::BeginTable("##stats", statCols, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_BordersInnerV)) {
        if (cpu) {
            std::snprintf(a, sizeof a, "%.0f%%", TotalCpu());                      StatCell("Utilization", a);
            std::snprintf(a, sizeof a, "%.2f GHz", g.s.cpuGhz * (0.85f + TotalCpu() / 400.0f));
                                                                                   StatCell("Speed", a);
            std::snprintf(a, sizeof a, "%.2f GHz", g.s.cpuGhz);                    StatCell("Base speed", a);
            std::snprintf(a, sizeof a, "%d", static_cast<int>(g.procs.size()));    StatCell("Processes", a);
            std::snprintf(a, sizeof a, "%d", g.s.cores);                           StatCell("Cores", a);
            std::snprintf(a, sizeof a, "%d", g.s.logical);                         StatCell("Logical processors", a);
            std::snprintf(a, sizeof a, "%d:%02d:%02d:%02d", upS / 86400, (upS / 3600) % 24, (upS / 60) % 60, upS % 60);
                                                                                   StatCell("Up time", a);
        } else {
            StatCell("In use", used);
            FormatMem(a, sizeof a, std::max(0.0f, g.s.totalMemMb - TotalMemMb())); StatCell("Available", a);
            StatCell("Total", total);
            std::snprintf(a, sizeof a, "%.0f%%", MemPercent());                    StatCell("Usage", a);
            std::snprintf(a, sizeof a, "%d", static_cast<int>(g.procs.size()));    StatCell("Processes", a);
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}

} // namespace

// ------------------------------------------------------------------ Public API
void Reset() { g.initialized = false; }

void DrawContents(AppState& state, AppWindow& /*self*/) {
    // Window was closed and reopened (not drawn last frame): start fresh,
    // like relaunching Task Manager. Ended tasks come back.
    const int frame = ImGui::GetFrameCount();
    if (frame - g.lastDrawnFrame > 1) g.initialized = false;
    g.lastDrawnFrame = frame;

    if (!g.initialized) Generate(state.config);
    Update();

    if (ImGui::BeginTabBar("##tm_tabs")) {
        const bool perfFirst = g.firstFrame && g.s.defaultTab == "performance";
        const bool procFirst = g.firstFrame && !perfFirst;
        if (ImGui::BeginTabItem("Processes", nullptr, procFirst ? ImGuiTabItemFlags_SetSelected : 0)) {
            DrawProcessesTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Performance", nullptr, perfFirst ? ImGuiTabItemFlags_SetSelected : 0)) {
            DrawPerformanceTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    g.firstFrame = false;
}

} // namespace TaskManager
