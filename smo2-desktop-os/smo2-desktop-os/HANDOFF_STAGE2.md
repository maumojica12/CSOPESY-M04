# Stage 2 Handoff: Shell (Taskbar and Placeholder Applications)

**From:** Member 2 (Lance)  **To:** Member 3 (Task Manager and Quiz Readiness)
**Gate date:** Wednesday, October 7, 2026

## Gate checklist

- [x] Fixed Taskbar renders at the top or bottom of the screen based on `taskbar.position`.
- [x] Three clickable buttons: Settings, File Explorer, Task Manager.
- [x] Running indicator shows a colored bar on open apps.
- [x] Clicking a button toggles its window open or closed.
- [x] App windows do not slide above the Taskbar when focused.
- [x] Clock and PWR button move out of the Taskbar's reserved space.
- [x] Task Manager opens an empty stub window with id `taskmanager`.
- [x] All Taskbar and application values (labels, position, sizes, colors) are read from `config.ini`.

## 1. What changed

- `src/shell/Taskbar.cpp` is now a working panel. It loads its own `[taskbar]` section in `Init`, then every frame it sets `state.reservedTop` or `state.reservedBottom` and draws one button per entry in `state.windows`.
- `src/apps/Apps.cpp` registers three apps in `RegisterApps` and renders two of them (`DrawSettings`, `DrawFileExplorer`). The third app, `taskmanager`, is deliberately left as a stub so Stage 3 only needs to fill in `drawContents`.
- `DrawAppWindows` now opens app windows with `ImGuiWindowFlags_NoBringToFrontOnFocus` and centers them on the usable area (viewport minus the reserved Taskbar strip). This is what keeps the Taskbar on top of focused app windows.
- `config.ini` gained three new sections: `[taskbar]`, `[apps]`, and `[about]`.

## 2. Z-order and focus (why this works)

The compositor already draws Desktop, then app windows, then the Taskbar. In Dear ImGui the last window drawn sits on top only until something else is focused. By marking every app window with `NoBringToFrontOnFocus`, clicking an app no longer raises it above the Taskbar, so the "panel on top" invariant is preserved even after the user interacts with an app.

The trade-off is that two open app windows will keep the stacking order they were drawn in, rather than the most-recently-clicked one. The spec does not require per-app window raising, so this is acceptable.

## 3. How to fill in the Task Manager (Stage 3)

The window is already registered. Open `src/apps/Apps.cpp`, find the Task Manager block in `RegisterApps`, and replace the lambda with your own draw function.

```cpp
// Current stub:
w.drawContents = [](AppState& s, AppWindow& self) {
    ImGui::TextDisabled("Task Manager (stub)");
    // ...
};

// Replace with:
w.drawContents = &DrawTaskManager;   // defined higher up in Apps.cpp
```

Do not change the id (`taskmanager`), the title key (`apps.taskmgr_title`), or the `defaultSize` keys. The quiz may vary any of these from `config.ini`.

## 4. New `config.ini` keys

| Section     | Keys |
|-------------|------|
| `[taskbar]` | position, height, button_width, button_spacing, side_padding, background, border, button_color, button_hover, button_active, text_color, indicator_color, indicator_thickness |
| `[apps]`    | settings_title, settings_icon, settings_width, settings_height, explorer_title, explorer_icon, explorer_width, explorer_height, taskmgr_title, taskmgr_icon, taskmgr_width, taskmgr_height |
| `[about]`   | device_name, processor, ram, system_type, device_id, product_id, edition, version, os_build, experience |

Any missing or misspelled key falls back to the default and prints one notice in the console.

## 5. Code snippets for the PPT (Member 4)

**Taskbar panel skeleton** `src/shell/Taskbar.cpp`
```cpp
const ImVec2 pos = atTop
    ? ImVec2(vp->Pos.x, vp->Pos.y)
    : ImVec2(vp->Pos.x, vp->Pos.y + vp->Size.y - height);
ImGui::SetNextWindowPos(pos);
ImGui::SetNextWindowSize(ImVec2(vp->Size.x, height));

const ImGuiWindowFlags flags =
    ImGuiWindowFlags_NoDecoration    |
    ImGuiWindowFlags_NoMove          |
    ImGuiWindowFlags_NoSavedSettings |
    ImGuiWindowFlags_NoBringToFrontOnFocus;

if (ImGui::Begin("##Taskbar", nullptr, flags)) {
    // one button per entry in state.windows
}
ImGui::End();
```

**Running indicator** `src/shell/Taskbar.cpp`
```cpp
if (ImGui::Button(label, ImVec2(buttonWidth, btnH)))
    w.open = !w.open;

if (w.open) {
    const ImVec2 bmin = ImGui::GetItemRectMin();
    const ImVec2 bmax = ImGui::GetItemRectMax();
    dl->AddRectFilled(
        ImVec2(bmin.x + 10.0f, bmin.y - 5.0f),
        ImVec2(bmax.x - 10.0f, bmin.y - 2.0f),
        indicatorColor, 1.5f);
}
```

**Registering an app** `src/apps/Apps.cpp`
```cpp
AppWindow w;
w.id          = "fileexplorer";
w.title       = state.config.getString("apps.explorer_title", "File Explorer");
w.icon        = state.config.getString("apps.explorer_icon",  "Files");
w.defaultSize = ImVec2(720.0f, 480.0f);
w.drawContents = &DrawFileExplorer;
state.windows.push_back(w);
```

## 6. Known limitations

- Window-to-window raising within apps is not supported (see Section 2). The Taskbar is the intended way to switch between apps.
- If the quiz sets `taskbar.height` very small (under about 24 px) the button text starts to clip. There is no clamp. Member 3 can add one if needed.
- The File Explorer sidebar and file list are static placeholders. The spec does not require real behavior from them.