# Stage 3 Handoff: Task Manager and Quiz Readiness

**From:** Member 3 (Raina)  **To:** Member 4 (Ven)
**Gate date:** Friday, October 9, 2026 (delivered Sunday, October 11)

## Gate checklist

- [x] Task Manager window closely resembles Windows Task Manager: Processes and Performance tabs, summary header, End task button.
- [x] Processes table: Name, PID, Status, CPU, Memory, Disk, Network; click any header to sort (click again to reverse); scrolls vertically and, when the window is narrow, sideways with the Name column frozen.
- [x] Dummy-data generator driven by `[taskmgr]` in `config.ini` (process count, CPU/memory ranges, refresh interval, fluctuation, seed, names, suspended count), with slight live fluctuation.
- [x] Performance tab: CPU and Memory side list with sparklines, live history graph, stats (utilization, speed, cores, logical processors, up time, memory in use/available).
- [x] Test matrix for every module: `docs/TEST_MATRIX.md`.
- [x] Three quiz presets: `presets/`.
- [ ] Test matrix run once on the real Windows build. **(Raina, before the freeze tag)**
- [ ] Code freeze tagged on `main` (`git tag smo2-freeze`). **(Raina)**

## 1. What changed

- **New:** `src/apps/TaskManager.h` and `src/apps/TaskManager.cpp` (Task Manager window body). CMake picks them up automatically; in Visual Studio, re-run "Generate Cache" once.
- `src/apps/Apps.cpp`: the Task Manager stub lambda is replaced with `w.drawContents = &TaskManager::DrawContents;`. Also, `DrawAppWindows` now clamps each app window's first size to the usable screen area, so a small `window.width/height` in the quiz never opens windows larger than the screen.
- `src/shell/Taskbar.cpp`: fixes the "Known limitation" from Stage 2. Taskbar height is clamped to at least the font height + 16 px and ImGui's minimum window height, so a tiny `taskbar.height` no longer clips button text or covers app title bars.
- `config.ini`: new `[taskmgr]` section. `apps.taskmgr_width/height` raised to 1150×640 so all 7 columns fit at `ui.scale = 2.0`.
- **New:** `docs/TEST_MATRIX.md`, `presets/*.ini`, this file.

## 2. Design (for the PPT written discussion)

The Task Manager is the third application registered in `RegisterApps()`. Like the other apps, its window frame, title (`apps.taskmgr_title`) and size (`apps.taskmgr_width/height`) are handled by `DrawAppWindows()`, and `TaskManager::DrawContents()` only draws the body, so it inherits the same z-order rule that keeps the Taskbar on top. The body follows the layout of Windows Task Manager. The **Processes** tab has a summary header (process count, total CPU %, memory % and GB, disk and network) above a sortable table with Name, PID, Status, CPU, Memory, Disk and Network columns. Its cells are tinted from pale yellow to deep orange by usage, the header row and Name column stay frozen while scrolling, and a selected row can be removed with End task. The **Performance** tab has a CPU/Memory side list with sparklines, a large live history graph for the selected resource, and a stats panel. All numbers are dummy values from a generator configured by the `[taskmgr]` section of `config.ini`. At start, each process gets a base CPU and memory value randomly chosen inside the configured ranges. Every `refresh_ms` milliseconds, each value moves a little toward a new random target around its base value, scaled by `fluctuation` and clamped to the range. A shared "system load" factor also swings up and down, so the total CPU graph rises and falls like a real machine instead of averaging out flat. Because Dear ImGui is immediate-mode, the window is simply rebuilt from this data every frame. The table re-sorts only when the data or the chosen column changes, and a list clipper draws only the visible rows, so even 500 processes stay smooth. The CPU name, speed and total memory default to the `[about]` values so Settings and Task Manager always agree. Every config value is sanitized (inverted ranges are swapped, counts and percentages are clamped, bad text falls back to defaults), so unexpected quiz parameters never crash the program. Closing and reopening the window starts a fresh process list, just like relaunching the real Task Manager.

## 3. New `config.ini` keys (`[taskmgr]`)

| Key | Default | Meaning |
|---|---|---|
| process_count | 14 | Number of processes (1–500) |
| cpu_min / cpu_max | 0 / 8 | Per-process CPU % range (0–100; swapped if inverted) |
| mem_min_mb / mem_max_mb | 20 / 600 | Per-process memory range in MB |
| refresh_ms | 1000 | Update interval; 0 = frozen |
| fluctuation | 0.15 | 0–1, how much values move each update |
| seed | 0 | 0 = random each run; any other number = same values every run |
| suspended_count | 1 | Processes shown as Suspended (0% CPU) |
| process_names | (empty) | Comma-separated custom names; repeats get "(2)", "(3)"… |
| cpu_name, cpu_ghz, total_memory_mb | from `[about]` | Performance tab hardware info |
| cores / logical_processors | 4 / 8 | Performance tab |
| history_length | 60 | Samples kept in the graphs |
| default_tab | processes | processes / performance |
| show_disk_network | true | Disk and Network columns and cards |
| heatmap | true | Yellow/orange usage cells |
| cpu_color, memory_color, disk_color, network_color, label_color | blue, purple, green, orange, gray | Colors (`#RRGGBB`, `#RRGGBBAA` or `r,g,b[,a]`) |
| heat_low / heat_high | #FFD940 / #FF731A | Usage-cell color from light to heavy usage |

## 4. Code snippets for the PPT (Member 4)

**Plugging the Task Manager into the app system** `src/apps/Apps.cpp`
```cpp
AppWindow w;
w.id          = "taskmanager";
w.title       = state.config.getString("apps.taskmgr_title", "Task Manager");
w.icon        = state.config.getString("apps.taskmgr_icon",  "Task Mgr");
w.defaultSize = ImVec2(state.config.getFloat("apps.taskmgr_width",  720.0f),
                       state.config.getFloat("apps.taskmgr_height", 520.0f));
w.drawContents = &TaskManager::DrawContents;   // Stage 3: apps/TaskManager.cpp
state.windows.push_back(w);
```

**Config-driven dummy-data generator** `src/apps/TaskManager.cpp`
```cpp
for (int i = 0; i < g.s.processCount; ++i) {
    Process p;
    p.name    = kDefaultNames[i % kDefaultNameCount];   // or names from config.ini
    p.pid     = 1000 + i * 4 + static_cast<int>(Rand(0, 3.99f));
    p.cpuBase = p.cpu = Rand(g.s.cpuMin, g.s.cpuMax);
    p.memBase = p.mem = Rand(g.s.memMin, g.s.memMax);
    g.procs.push_back(p);
}
```

**Live fluctuation every `refresh_ms`** `src/apps/TaskManager.cpp`
```cpp
void Fluctuate() {
    const float f = g.s.fluctuation;
    g.load += Rand(-f, f) * 1.2f + (1.0f - g.load) * 0.25f;   // shared system-load swing
    g.load = std::clamp(g.load, 1.0f - f, 1.0f + f);
    for (auto& p : g.procs) {
        if (p.suspended) continue;
        const float target = p.cpuBase * g.load * (1.0f + Rand(-f, f));
        p.cpu = std::clamp(p.cpu + (target - p.cpu) * 0.6f, g.s.cpuMin, g.s.cpuMax);
    }
    PushHistory();          // feeds the Performance graph
    g.needsSort = true;     // keep the table sorted as values change
}
```

**Sortable, scrollable Processes table** `src/apps/TaskManager.cpp`
```cpp
if (ImGui::BeginTable("##processes", cols, ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY |
                      ImGuiTableFlags_ScrollX | ImGuiTableFlags_RowBg, ImVec2(0, -footer), innerW)) {
    ImGui::TableSetupScrollFreeze(1, 1);   // header row + Name column stay visible
    ImGui::TableSetupColumn("CPU", ImGuiTableColumnFlags_DefaultSort, wCpu, ColCpu);
    // ...
    if (ImGuiTableSortSpecs* specs = ImGui::TableGetSortSpecs())
        if (specs->SpecsDirty) { g.sortColumn = specs->Specs[0].ColumnUserID; g.needsSort = true; }
    ImGuiListClipper clipper;                 // only visible rows are drawn
    clipper.Begin(static_cast<int>(g.order.size()));
```

## 5. Testing done

- Every file compiles with no warnings (`-Wall -Wextra`) against Dear ImGui v1.91.8, the version pinned in `CMakeLists.txt`.
- A headless test ran the real `Config`, `Apps`, `Taskbar` and `config.ini` under AddressSanitizer with no errors. It checked every `[taskmgr]` row of the test matrix with exact numeric checks: ranges hold under maximum fluctuation, refresh rate, frozen mode, seed repeatability, custom names, suspended count, sort order in all 7 columns in both directions, sort kept across refreshes, End task, reopen reset, 500 processes, and garbage values.
- Layout was checked from rendered frames at `ui.scale` 1.0 and 2.0, and at 854×480 with a 10 px top taskbar. No text is cut off; the table scrolls sideways when the window is too narrow.

## 6. Known limitations

- Values are dummy data by design (the spec does not ask for real process information).
- End task only removes the row; reopening Task Manager brings it back.
- Only Processes and Performance tabs exist (the spec names those two as examples).
