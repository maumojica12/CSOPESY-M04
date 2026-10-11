#include "shell/Taskbar.h"

#include <algorithm>
#include <string>

#include "imgui.h"
#include "core/AppState.h"

namespace {

struct TaskbarStyle {
    std::string position;
    float       height;
    float       buttonWidth;
    float       buttonSpacing;
    float       sidePadding;
    ImU32       background;
    ImU32       border;
    ImU32       buttonIdle;
    ImU32       buttonHover;
    ImU32       buttonActive;
    ImU32       textColor;
    ImU32       indicatorColor;
    float       indicatorThickness;
    bool        loaded = false;
};

TaskbarStyle g_style;

void LoadStyle(const Config& cfg) {
    g_style.position           = cfg.getChoice("taskbar.position", "bottom");
    g_style.height             = cfg.getFloat ("taskbar.height", 56.0f);
    g_style.buttonWidth        = cfg.getFloat ("taskbar.button_width", 120.0f);
    g_style.buttonSpacing      = cfg.getFloat ("taskbar.button_spacing", 8.0f);
    g_style.sidePadding        = cfg.getFloat ("taskbar.side_padding", 12.0f);
    g_style.background         = cfg.getColor ("taskbar.background",      IM_COL32(18, 20, 28, 235));
    g_style.border             = cfg.getColor ("taskbar.border",          IM_COL32(255, 255, 255, 25));
    g_style.buttonIdle         = cfg.getColor ("taskbar.button_color",    IM_COL32(255, 255, 255, 16));
    g_style.buttonHover        = cfg.getColor ("taskbar.button_hover",    IM_COL32(255, 255, 255, 36));
    g_style.buttonActive       = cfg.getColor ("taskbar.button_active",   IM_COL32(255, 255, 255, 60));
    g_style.textColor          = cfg.getColor ("taskbar.text_color",      IM_COL32(240, 240, 240, 255));
    g_style.indicatorColor     = cfg.getColor ("taskbar.indicator_color", IM_COL32(80, 180, 255, 255));
    g_style.indicatorThickness = cfg.getFloat ("taskbar.indicator_thickness", 3.0f);
    g_style.loaded             = true;
}

} // namespace

void Taskbar::Init(AppState& state) {
    LoadStyle(state.config);
}

void Taskbar::Draw(AppState& state) {
    if (!g_style.loaded) LoadStyle(state.config);

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const bool  atTop  = (g_style.position == "top");
    // Clamp so button text never clips if the quiz sets a tiny height
    // (buttons are height - 12, so this leaves room for one line of text).
    // Also respect ImGui's minimum window height, or the panel ends up taller
    // than the space it reserves and covers app title bars.
    const float height = std::max({ g_style.height, ImGui::GetFontSize() + 16.0f,
                                    ImGui::GetStyle().WindowMinSize.y });

    // Reserve screen space so the clock and PWR button move out of the way.
    if (atTop) { state.reservedTop = height; state.reservedBottom = 0.0f; }
    else       { state.reservedBottom = height; state.reservedTop = 0.0f; }

    const ImVec2 pos  = atTop
        ? ImVec2(vp->Pos.x, vp->Pos.y)
        : ImVec2(vp->Pos.x, vp->Pos.y + vp->Size.y - height);
    const ImVec2 size = ImVec2(vp->Size.x, height);

    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSize(size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,    ImVec2(g_style.sidePadding, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,   0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg,            g_style.background);

    // NoBringToFrontOnFocus keeps the panel from stealing focus. Because the
    // taskbar is drawn LAST each frame and app windows also set this flag,
    // nothing can slide above it.
    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration     |
        ImGuiWindowFlags_NoMove           |
        ImGuiWindowFlags_NoSavedSettings  |
        ImGuiWindowFlags_NoNav            |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("##Taskbar", nullptr, flags)) {
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // Thin accent line on the edge that faces the desktop.
        const float borderY = atTop ? (pos.y + size.y) : pos.y;
        dl->AddLine(ImVec2(pos.x, borderY), ImVec2(pos.x + size.x, borderY),
                    g_style.border, 1.0f);

        // Center the row of app buttons horizontally.
        const int   count  = static_cast<int>(state.windows.size());
        const float rowW   = count > 0
            ? count * g_style.buttonWidth + (count - 1) * g_style.buttonSpacing
            : 0.0f;
        const float startX = (size.x - rowW) * 0.5f;
        const float btnH   = height - 12.0f;
        const float btnY   = (height - btnH) * 0.5f;

        ImGui::PushStyleColor(ImGuiCol_Button,        g_style.buttonIdle);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, g_style.buttonHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  g_style.buttonActive);
        ImGui::PushStyleColor(ImGuiCol_Text,          g_style.textColor);
        ImGui::PushStyleVar  (ImGuiStyleVar_FrameRounding, 6.0f);

        for (int i = 0; i < count; ++i) {
            AppWindow& w = state.windows[i];
            const float x = startX + i * (g_style.buttonWidth + g_style.buttonSpacing);

            ImGui::SetCursorPos(ImVec2(x, btnY));
            const std::string label = w.icon.empty() ? w.title : w.icon;
            const std::string btnId = label + "##tb_" + w.id;

            if (ImGui::Button(btnId.c_str(), ImVec2(g_style.buttonWidth, btnH))) {
                w.open = !w.open;
            }

            // Running indicator: colored bar on the edge nearest the desktop.
            if (w.open) {
                const ImVec2 bmin = ImGui::GetItemRectMin();
                const ImVec2 bmax = ImGui::GetItemRectMax();
                const float  t    = g_style.indicatorThickness;
                if (atTop) {
                    dl->AddRectFilled(
                        ImVec2(bmin.x + 10.0f, bmax.y + 2.0f),
                        ImVec2(bmax.x - 10.0f, bmax.y + 2.0f + t),
                        g_style.indicatorColor, t * 0.5f);
                } else {
                    dl->AddRectFilled(
                        ImVec2(bmin.x + 10.0f, bmin.y - t - 2.0f),
                        ImVec2(bmax.x - 10.0f, bmin.y - 2.0f),
                        g_style.indicatorColor, t * 0.5f);
                }
            }
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
}