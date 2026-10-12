# CSOPESY Semi-Major Output 2: Desktop-Style OS Mock-up

## Group Members

1. `Lance Krystofer A. Galicia``Raina B. Helaga`
2. `Raina B. Helaga`
3. `Maurienne Maria M. Mojia`
4. `Venice Raeka M. Plurad`

**Section:** `S02`

## Entry File

`src/main.cpp` (contains `main()`)

## Stack

- GLFW (window and input), OpenGL 3 (rendering), Dear ImGui (UI)
- Language: C++17. Build system: CMake.
- Libraries (GLFW 3.4, Dear ImGui v1.91.8, stb_image) are downloaded automatically by CMake FetchContent on the first configure. An internet connection is required for that first configure.

## How to Run (Visual Studio 2022)

1. Make sure the **Desktop development with C++** workload is installed (Visual Studio Installer > Modify). It includes **C++ CMake tools for Windows**.
2. Open Visual Studio and choose **File > Open > Folder...**. Select the folder that directly contains `CMakeLists.txt` (the `smo2-desktop-os` folder, not its parent).
3. Wait for **"CMake generation finished."** in the Output window (View > Output, then choose **CMake** in the "Show output from" list). The first run downloads the libraries and may take a few minutes.
4. In the toolbar, pick **DesktopOS.exe** from the startup item dropdown beside the green Run button.
5. Press **F5** (Run/Debug). The desktop window opens.

To close the program, click the **PWR** button and confirm **Shut Down**. Alt+F4 and the window's X button are intentionally blocked, and a notice ("Use the PWR button to shut down.") appears instead.

If no startup item appears, use **Project > Configure Cache** (or **Project > Delete Cache and Reconfigure**) and check the Output window.

## Features

### Desktop (`src/desktop/Desktop.cpp`)

- Full-window wallpaper: solid, gradient (vertical, horizontal, diagonal), drawn pattern (grid, dots, stripes), or image texture.
- Real-time clock, updated every frame, in a configurable corner, with 12/24-hour format and optional seconds and date.
- PWR button: the only way to exit, with an optional confirmation dialog.

### Taskbar (`src/shell/Taskbar.cpp`)

- Fixed panel at the top or bottom (no move, resize, or title bar), always drawn above application windows.
- One icon button per application: Settings, File Explorer, Task Manager. Each button opens and closes its window.
- A colored bar shows which applications are running.

### Applications (`src/apps/`)

- **Settings ("About this PC"):** placeholder system information.
- **File Explorer:** address bar, sidebar, and a file table with placeholder entries.
- **Task Manager (`TaskManager.cpp`):**
  - Processes tab: Name, PID, Status, CPU, Memory, Disk, Network. Sortable and scrollable, with heat-map cells and an End task button.
  - Performance tab: CPU and Memory graphs.
  - Values are dummy data that fluctuate slightly on each refresh.

## Render Order

Every frame, in `main.cpp`:

1. `Desktop::Draw`: wallpaper, clock, PWR button (bottom layer)
2. `DrawAppWindows`: application windows (middle layer)
3. `Taskbar::Draw`: system panel (top layer)

All modules share one `AppState` (`src/core/AppState.h`), which holds the config, the running flag, the open/closed window list, and the screen space reserved by the taskbar.

## Configuration (`config.ini`)

All adjustable values are in `config.ini` in the project folder. **No recompiling is needed.** Edit the file, save it, then press Run again.

**Config lookup order:**

1. Path given as the first command-line argument
2. `config.ini` in the current working directory
3. `config.ini` in the project folder (default when running from the IDE)

**Format notes:**

- Keys are written as `[section]` then `key = value`, and read in code as `section.key`.
- Comments start with `;` or `#`.
- Colors: `#RRGGBB`, `#RRGGBBAA`, or `r,g,b[,a]` (0-255).
- Corners: `top_left`, `top_right`, `bottom_left`, `bottom_right`.
- Missing or invalid values fall back to safe defaults and print a one-time notice in the console.

**Sections:**

| Section | Keys |
|---|---|
| `[window]` | title, mode (windowed, borderless, fullscreen), width, height, resizable, vsync, block_close |
| `[ui]` | theme (dark, light, classic), scale |
| `[desktop]` | wallpaper_mode (solid, gradient, pattern, image), colors, gradient_direction, pattern_*, image_path, image_fit, margin |
| `[clock]` | enabled, corner, format_24h, show_seconds, show_date, scale, color |
| `[pwr]` | label, corner, color, confirm |
| `[taskbar]` | position (top, bottom), height, button_width, button_spacing, side_padding, colors, indicator_color, indicator_thickness |
| `[apps]` | title, icon, width, and height of each application window |
| `[about]` | placeholder system info shown in Settings (also used as defaults by Task Manager) |
| `[taskmgr]` | process_count, cpu_min, cpu_max, mem_min_mb, mem_max_mb, refresh_ms (0 = frozen), fluctuation, seed (0 = random), suspended_count, process_names, cores, logical_processors, history_length, default_tab (processes, performance), show_disk_network, heatmap, colors |

**Wallpaper image mode:** place the image at `assets/wallpaper.png` (or set `desktop.image_path`, relative to `config.ini`). If it fails to load, the gradient is used instead.

**Default recording setup:** window 1600 x 900, UI scale 2.0. For a 720p recording, set `width = 1280` and `height = 720` in `[window]`.

## Project Structure

```
CMakeLists.txt
config.ini
src/
  main.cpp                entry point, main loop, clean shutdown
  core/     AppState.h, Config.h/.cpp, Texture.h/.cpp
  desktop/  Desktop.h/.cpp
  shell/    Taskbar.h/.cpp
  apps/     Apps.h/.cpp, TaskManager.h/.cpp
```

## Notes

- No `imgui.ini` is written, so every run starts from the same layout.
- Closing and reopening the Task Manager regenerates its dummy data.
- Console messages (`[Config]`, `[Desktop]`, `[System]`) show what was loaded.
