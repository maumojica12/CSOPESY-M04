# Stage 1 Handoff — Foundation: Core Framework and Desktop Layer

**From:** Member 1  **To:** Member 2 (Shell: Taskbar and Placeholder Applications)
**Gate date:** Monday, October 5, 2026

## Gate checklist

- [x] Program runs from Run/Debug and shows the Desktop with wallpaper and clock.
- [x] The program closes only through PWR (title-bar X / Alt+F4 are blocked and show a notice).
- [x] AppState and the configuration loader are on `main`; see "How to add" below.
- [x] Handoff notes and code snippets for documentation are below.

## 1. Architecture (for the PPT design discussion)

The desktop OS is a single-window GLFW application that renders with OpenGL 3 and
draws its entire interface with Dear ImGui. Because Dear ImGui is immediate-mode,
nothing on screen persists between frames: every frame, the main loop rebuilds the
whole screen from one shared `AppState` structure, which holds the system's
lifecycle flag, frame timing, the list of application windows with their open or
closed state, and the screen space reserved by the taskbar. `main.cpp` acts as the
compositor and draws three layers in a fixed order: the Desktop (wallpaper, clock,
and PWR button, rendered on ImGui's background draw list so it is always behind
everything), the application windows, and finally the Taskbar. All visible
parameters are read once at startup from `config.ini` through the `Config` class,
which allows the appearance and behavior to be changed during the quiz without
recompiling. The main loop runs only while `state.running` is true; the PWR button
is the only code that sets it to false, after which the program releases the
wallpaper texture, shuts down ImGui, and destroys the GLFW window before exiting.
Any other close request from the operating system is intercepted by a GLFW
close callback and cancelled.

## 2. Folder structure

```
config.ini                 <- all quiz parameters (edit, save, Run; no rebuild)
assets/wallpaper.png       <- sample wallpaper for image mode
src/main.cpp               <- entry point, window setup, compositor loop, shutdown
src/core/AppState.h        <- shared state + AppWindow definition
src/core/Config.h/.cpp     <- config.ini loader
src/core/Texture.h/.cpp    <- image -> OpenGL texture (stb_image)
src/desktop/Desktop.*      <- wallpaper, clock, PWR button, close notice   (Stage 1, done)
src/apps/Apps.*            <- RegisterApps() + DrawAppWindows()            (Stage 2 fills RegisterApps)
src/shell/Taskbar.*        <- stub with TODO list                          (Stage 2)
```

New `.cpp/.h` files anywhere under `src/` are picked up automatically by CMake
(re-run CMake configure / "Generate Cache" in Visual Studio after adding files).

## 3. How to add a new application window (Stage 2)

Register it once in `RegisterApps()` in `src/apps/Apps.cpp`. The frame (title bar,
close button, size, centered position) is handled for you; you only draw the body.

```cpp
AppWindow settings;
settings.id          = "settings";                                   // unique, lowercase
settings.title       = state.config.getString("apps.settings_title", "Settings");
settings.icon        = "SET";                                         // taskbar label
settings.defaultSize = ImVec2(560, 400);
settings.drawContents = [](AppState& s, AppWindow& self) {
    ImGui::Text("Placeholder settings page");
};
state.windows.push_back(settings);
```

To open/close it from the taskbar: `w.open = !w.open;`
To check if it is running (taskbar indicator): `if (w.open) ...`
To find one by id: `state.findWindow("taskmanager")`.

**Stage 2 must also register the Task Manager stub** with id `taskmanager`
so Stage 3 only fills in `drawContents`.

## 4. How to add a new configuration key

1. Add it to `config.ini` under a section, with a comment explaining valid values:
   ```ini
   [taskbar]
   position = bottom   ; top | bottom
   ```
2. Read it once in your module's `Init()` with a default:
   ```cpp
   std::string pos = state.config.getChoice("taskbar.position", "bottom");
   float height    = state.config.getFloat("taskbar.height", 48.0f);
   ImU32 color     = state.config.getColor("taskbar.color", IM_COL32(20, 20, 30, 230));
   ```
Available getters: `getString`, `getChoice` (lowercased), `getInt`, `getFloat`,
`getBool`, `getColor` (`#RRGGBB`, `#RRGGBBAA`, or `r,g,b[,a]`), and
`resolvePath` for file paths relative to `config.ini`.
A missing or misspelled key never crashes; it uses the default and prints a notice.

## 5. Contract with the Taskbar (Stage 2)

- `Taskbar::Draw()` is already called last every frame in `main.cpp`.
- Every frame, set `state.reservedBottom` (or `state.reservedTop`) to the taskbar's
  height. The clock and PWR button move out of the way automatically.
- Use `ImGui::GetMainViewport()` for the screen position and size.

## 6. Current config.ini keys (Stage 1)

| Section   | Keys |
|-----------|------|
| `[window]`  | title, mode (windowed/borderless/fullscreen), width, height, resizable, vsync, block_close |
| `[ui]`      | theme (dark/light/classic), scale |
| `[desktop]` | wallpaper_mode (solid/gradient/pattern/image), margin, solid_color, gradient_top, gradient_bottom, gradient_direction (vertical/horizontal/diagonal), pattern_type (grid/dots/stripes), pattern_color, pattern_spacing, pattern_thickness, pattern_speed, image_path, image_fit (fill/stretch) |
| `[clock]`   | enabled, corner, format_24h, show_seconds, show_date, scale, color |
| `[pwr]`     | label, corner, color, confirm |

## 7. Code snippets for the PPT (Member 4)

**Compositor render loop** — `src/main.cpp`
```cpp
while (state.running) {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    Desktop::Draw(state);     // 1. bottom layer
    DrawAppWindows(state);    // 2. application windows
    Taskbar::Draw(state);     // 3. top layer

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}
```

**Configuration loader** — `src/core/Config.cpp`
```cpp
const std::string key = Lower(Trim(line.substr(0, eq)));
std::string value = Trim(line.substr(eq + 1));
values_[section.empty() ? key : section + "." + key] = value;
// ...
int Config::getInt(const std::string& key, int def) const {
    if (const std::string* v = find(key)) {
        try { return std::stoi(*v); } catch (...) {}
    }
    noteDefault(key);
    return def;
}
```

**PWR-only shutdown** — `src/main.cpp` and `src/desktop/Desktop.cpp`
```cpp
void BlockCloseCallback(GLFWwindow* window) {          // X button / Alt+F4
    glfwSetWindowShouldClose(window, GLFW_FALSE);
    static_cast<AppState*>(glfwGetWindowUserPointer(window))->closeBlockedTimer = 3.0f;
}

if (ImGui::Button("Shut Down", ImVec2(130, 0))) {      // PWR confirmation dialog
    state.running = false;                             // ends the main loop
}

// after the loop: clean shutdown
Desktop::Shutdown(state);
ImGui_ImplOpenGL3_Shutdown();
ImGui_ImplGlfw_Shutdown();
ImGui::DestroyContext();
glfwDestroyWindow(window);
glfwTerminate();
```

Screenshots of every wallpaper mode, the PWR confirmation, and the close-blocked
notice are in `docs/screenshots/`.

## 8. Known limitations

- Closing the separate console window (Windows) ends the process; that is outside
  the GLFW window and cannot be blocked. Do not close it during the recording.
- In `fullscreen` mode some screen recorders capture a black screen; use
  `windowed` (1280x720) for the video.
