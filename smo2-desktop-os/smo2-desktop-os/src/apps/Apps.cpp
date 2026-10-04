#include "apps/Apps.h"

#include <string>
#include "imgui.h"
#include "core/AppState.h"

void RegisterApps(AppState& state) {
    // >>> STAGE 2 (Member 2) registers the applications here. <<<
    // Each entry automatically gets a window frame (title bar, close button,
    // default size and position). Only the BODY is drawn by drawContents.
    //
    // Example:
    // AppWindow settings;
    // settings.id          = "settings";
    // settings.title       = state.config.getString("apps.settings_title", "Settings");
    // settings.icon        = "SET";
    // settings.defaultSize = ImVec2(560, 400);
    // settings.drawContents = [](AppState& s, AppWindow& self) {
    //     ImGui::Text("Placeholder settings page");
    // };
    // state.windows.push_back(settings);
    (void)state;
}

void DrawAppWindows(AppState& state) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();

    for (AppWindow& w : state.windows) {
        if (!w.open) continue;

        ImGui::SetNextWindowSize(w.defaultSize, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

        // "Title###id": the title can change (e.g. from config) without changing the window's identity.
        const std::string label = w.title + "###" + w.id;
        if (ImGui::Begin(label.c_str(), &w.open)) {
            if (w.drawContents) w.drawContents(state, w);
        }
        ImGui::End();
    }
}
