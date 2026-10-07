#include "apps/Apps.h"

#include <string>
#include "imgui.h"
#include "core/AppState.h"

namespace {

// ----- Placeholder 1: Settings / About This PC ------------------------------
void DrawSettings(AppState& s, AppWindow& /*self*/) {
    const Config& cfg = s.config;
    ImGui::TextDisabled("About this PC");
    ImGui::Separator();
    ImGui::Spacing();

    auto row = [](const char* label, const std::string& value) {
        ImGui::Text("%s", label);
        ImGui::SameLine(200.0f);
        ImGui::TextUnformatted(value.c_str());
    };

    row("Device name:",   cfg.getString("about.device_name",  "CSOPESY-PC"));
    row("Processor:",     cfg.getString("about.processor",    "Virtual CPU @ 3.20 GHz"));
    row("Installed RAM:", cfg.getString("about.ram",          "8.00 GB"));
    row("System type:",   cfg.getString("about.system_type",  "64-bit operating system"));
    row("Device ID:",     cfg.getString("about.device_id",    "SMO2-0001-DLSU"));
    row("Product ID:",    cfg.getString("about.product_id",   "00000-11111-22222-AAAAA"));

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextDisabled("System specifications");
    ImGui::Spacing();

    row("Edition:",       cfg.getString("about.edition",      "CSOPESY Desktop OS"));
    row("Version:",       cfg.getString("about.version",      "SMO2 (Build 2026.10)"));
    row("OS build:",      cfg.getString("about.os_build",     "SMO2.0.1.0"));
    row("Experience:",    cfg.getString("about.experience",   "Dear ImGui shell"));
}

// ----- Placeholder 2: File Explorer -----------------------------------------
void DrawFileExplorer(AppState& /*s*/, AppWindow& /*self*/) {
    static char addr[256] = "This PC > Documents";
    ImGui::PushItemWidth(-1);
    ImGui::InputText("##addr", addr, IM_ARRAYSIZE(addr), ImGuiInputTextFlags_ReadOnly);
    ImGui::PopItemWidth();
    ImGui::Spacing();

    const float sidebarW = 170.0f;
    ImGui::BeginChild("##sidebar", ImVec2(sidebarW, 0), true);
    ImGui::TextDisabled("Quick access");
    const char* quick[] = { "Desktop", "Downloads", "Documents",
                            "Pictures", "Music", "Videos" };
    for (const char* f : quick) ImGui::Selectable(f);
    ImGui::Spacing();
    ImGui::TextDisabled("This PC");
    ImGui::Selectable("Local Disk (C:)");
    ImGui::Selectable("Data (D:)");
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("##files", ImVec2(0, 0), true);
    if (ImGui::BeginTable("files", 4,
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
            ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Date modified");
        ImGui::TableSetupColumn("Type");
        ImGui::TableSetupColumn("Size");
        ImGui::TableHeadersRow();

        struct Entry { const char* name; const char* date; const char* type; const char* size; };
        const Entry rows[] = {
            { "Project Report.docx", "10/06/2026 14:21", "Word document",      "42 KB"  },
            { "Budget.xlsx",         "10/05/2026 09:03", "Excel spreadsheet",  "18 KB"  },
            { "Lecture Notes.pdf",   "10/04/2026 20:14", "PDF document",       "612 KB" },
            { "wallpaper.png",       "09/28/2026 11:42", "PNG image",          "187 KB" },
            { "notes.txt",           "09/27/2026 08:11", "Text document",      "2 KB"   },
            { "setup.exe",           "08/15/2026 16:07", "Application",        "14 MB"  },
            { "README.md",           "10/01/2026 10:00", "Markdown",           "3 KB"   },
            { "archive.zip",         "07/19/2026 22:30", "Compressed archive", "4 MB"   },
        };
        for (const Entry& r : rows) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(r.name);
            ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(r.date);
            ImGui::TableSetColumnIndex(2); ImGui::TextUnformatted(r.type);
            ImGui::TableSetColumnIndex(3); ImGui::TextUnformatted(r.size);
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
}

} // namespace

void RegisterApps(AppState& state) {
    // Placeholder 1: Settings
    {
        AppWindow w;
        w.id          = "settings";
        w.title       = state.config.getString("apps.settings_title", "Settings");
        w.icon        = state.config.getString("apps.settings_icon",  "Settings");
        w.defaultSize = ImVec2(state.config.getFloat("apps.settings_width",  620.0f),
                               state.config.getFloat("apps.settings_height", 460.0f));
        w.drawContents = &DrawSettings;
        state.windows.push_back(w);
    }

    // Placeholder 2: File Explorer
    {
        AppWindow w;
        w.id          = "fileexplorer";
        w.title       = state.config.getString("apps.explorer_title", "File Explorer");
        w.icon        = state.config.getString("apps.explorer_icon",  "Files");
        w.defaultSize = ImVec2(state.config.getFloat("apps.explorer_width",  720.0f),
                               state.config.getFloat("apps.explorer_height", 480.0f));
        w.drawContents = &DrawFileExplorer;
        state.windows.push_back(w);
    }

    // Task Manager stub (Member 3 fills drawContents in Stage 3)
    {
        AppWindow w;
        w.id          = "taskmanager";
        w.title       = state.config.getString("apps.taskmgr_title", "Task Manager");
        w.icon        = state.config.getString("apps.taskmgr_icon",  "Task Mgr");
        w.defaultSize = ImVec2(state.config.getFloat("apps.taskmgr_width",  720.0f),
                               state.config.getFloat("apps.taskmgr_height", 520.0f));
        w.drawContents = [](AppState& /*s*/, AppWindow& /*self*/) {
            ImGui::TextDisabled("Task Manager (stub)");
            ImGui::Separator();
            ImGui::TextWrapped("Placeholder window for Stage 3. Member 3 will "
                               "add the Processes table, Performance tab, and "
                               "dummy-data generator here.");
        };
        state.windows.push_back(w);
    }
}

void DrawAppWindows(AppState& state) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();

    // NoBringToFrontOnFocus keeps focused app windows from jumping above the
    // taskbar, which is drawn later in the frame.
    const ImGuiWindowFlags appFlags = ImGuiWindowFlags_NoBringToFrontOnFocus;

    for (AppWindow& w : state.windows) {
        if (!w.open) continue;

        // Center on the usable area (viewport minus the reserved taskbar strip).
        const ImVec2 usableCenter(
            vp->Pos.x + vp->Size.x * 0.5f,
            vp->Pos.y + state.reservedTop +
                (vp->Size.y - state.reservedTop - state.reservedBottom) * 0.5f);

        ImGui::SetNextWindowSize(w.defaultSize, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(usableCenter, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

        const std::string label = w.title + "###" + w.id;
        if (ImGui::Begin(label.c_str(), &w.open, appFlags)) {
            if (w.drawContents) w.drawContents(state, w);
        }
        ImGui::End();
    }
}