# SMO2 Test Matrix (Stage 3: Mock Quiz)

**How to run:** change `config.ini` only, save, press Run/Debug (F5), and check the expected result. Never touch code during the run (same rule as the real quiz). Fill in **Result** with Pass/Fail and a short note.

**Tip:** keys that appear later in `config.ini` win. To try a test quickly, add a new section at the very bottom (e.g. `[taskmgr]` then `process_count = 100`) and delete it afterwards. Presets in `presets/` work the same way: paste one at the bottom of `config.ini`.

Tester: ______  Date: ______  Commit: ______

## Window and UI (`[window]`, `[ui]`)

| # | Change | Expected | Result |
|---|---|---|---|
| W1 | `width = 1280`, `height = 720` | Window opens at 1280×720 | |
| W2 | `width = 854`, `height = 480` | Everything still fits; app windows open no larger than the screen; Task Manager table scrolls sideways | |
| W3 | `mode = borderless` | Fills the monitor, no title bar; only PWR closes it | |
| W4 | `mode = fullscreen` | Fullscreen; PWR still closes cleanly | |
| W5 | `title = My OS` | Title bar text changes | |
| W6 | `block_close = true`, click the X / press Alt+F4 | Window stays open; "use PWR" notice shows | |
| W7 | `[ui] theme = light`, then `classic`, then `dark` | Theme changes | |
| W8 | `[ui] scale = 1.0`, then `2.5` | Text and widgets resize; nothing overlaps; Task Manager columns never cut text | |
| W9 | Rename `config.ini` temporarily | Program still starts with built-in defaults | |

## Desktop (`[desktop]`, `[clock]`, `[pwr]`)

| # | Change | Expected | Result |
|---|---|---|---|
| D1 | `wallpaper_mode = solid`, `solid_color = #2E7D32` | Solid green wallpaper | |
| D2 | `wallpaper_mode = gradient`, `gradient_direction = horizontal` / `diagonal` | Gradient direction changes | |
| D3 | `wallpaper_mode = pattern`, `pattern_type = dots` / `stripes`, `pattern_speed = 30` | Pattern changes and drifts | |
| D4 | `wallpaper_mode = image` | `assets/wallpaper.png` fills the screen | |
| D5 | `image_path = assets/missing.png` | No crash; falls back (see console notice) | |
| D6 | `image_fit = stretch` | Image stretches instead of cropping | |
| D7 | `[clock] corner = bottom_left` (then the other 3 corners) | Clock moves; never hidden under the taskbar | |
| D8 | `format_24h = false`, `show_seconds = false`, `show_date = false` | 12-hour clock, no seconds, no date | |
| D9 | `[clock] enabled = false` | No clock | |
| D10 | `[clock] scale = 4`, `color = #FFD54F` | Bigger, yellow clock | |
| D11 | `[pwr] corner = bottom_right`, `label = OFF`, `color = #1565C0` | PWR button moves, relabels, recolors | |
| D12 | `[pwr] confirm = false`, click PWR | Shuts down immediately; console prints "Shutdown complete." | |
| D13 | `[pwr] confirm = true`, click PWR | "Are you sure?" dialog first; Cancel keeps it running | |

## Taskbar and apps (`[taskbar]`, `[apps]`, `[about]`)

| # | Change | Expected | Result |
|---|---|---|---|
| T1 | `[taskbar] position = top` | Taskbar at top; clock/PWR move down; app title bars not covered | |
| T2 | `height = 80`, then `height = 10` | Taller bar; at 10 it clamps so button text still fits and no window hides under it | |
| T3 | `button_width = 200`, `button_spacing = 30` | Wider, more spaced buttons, still centered | |
| T4 | `background = #000000`, `indicator_color = #FF5252`, `indicator_thickness = 6` | Colors/indicator change | |
| T5 | Click each of the 3 buttons twice | Opens, then closes its window; running indicator toggles | |
| T6 | Open all 3 windows at once, click each | All usable; taskbar stays on top | |
| T7 | `[apps] settings_title = System`, `explorer_icon = Explorer`, `taskmgr_icon = TM` | Titles/labels change | |
| T8 | `[apps] taskmgr_width = 700`, `taskmgr_height = 450` | Task Manager opens at that size | |
| T9 | `[about] device_name = QUIZ-PC`, `processor = Intel i7 @ 2.60 GHz`, `ram = 16 GB` | Settings shows these; Task Manager Performance shows the same CPU name, 2.60 GHz and 16 GB | |

## Task Manager (`[taskmgr]`)

| # | Change | Expected | Result |
|---|---|---|---|
| M1 | Defaults | 14 processes (1 suspended), Processes tab, values change every ~1 s, CPU sorted high→low | |
| M2 | `process_count = 1` | One row; summary shows 1 | |
| M3 | `process_count = 100` | Table scrolls; header row and Name column stay frozen | |
| M4 | `cpu_min = 20`, `cpu_max = 30` | Every running process stays within 20.0–30.0% (suspended rows 0%) | |
| M5 | `cpu_min = 50`, `cpu_max = 5` (inverted) | No crash; treated as 5–50 | |
| M6 | `mem_min_mb = 1000`, `mem_max_mb = 4000` | Memory values between 1000 MB and 3.9 GB (shows GB above 1024 MB) | |
| M7 | `refresh_ms = 250` | Values change ~4 times a second | |
| M8 | `refresh_ms = 0` | Values frozen; footer says "Refresh: paused" | |
| M9 | `fluctuation = 0`, then `fluctuation = 1` | 0 = steady values; 1 = big jumps, CPU graph swings | |
| M10 | `seed = 1234`, run twice | Same names, PIDs and starting values both runs | |
| M11 | `process_names = Chrome, Spotify, Discord`, `process_count = 5` | Chrome, Spotify, Discord, Chrome (2), Spotify (2) | |
| M12 | `suspended_count = 3` | Summary says 3 fewer running; 3 rows "Suspended" with 0% CPU | |
| M13 | `total_memory_mb = 4096` | Memory % rises; Performance > Memory shows 4.0 GB total | |
| M14 | `default_tab = performance` | Opens on the Performance tab | |
| M15 | `show_disk_network = false` | Disk and Network columns and cards disappear | |
| M16 | `heatmap = false` | No colored usage cells | |
| M17 | `cores = 2`, `logical_processors = 4`, `cpu_name = Pentium 4 @ 1.50 GHz` | Performance > CPU shows these (GHz read from the name) | |
| M18 | `history_length = 10` | Graphs show only the last 10 samples | |
| M19 | Click CPU / Memory / Name / PID / Status / Disk / Network headers | Sorts; click again reverses; stays sorted while values refresh | |
| M20 | Select a row, click End task | Row disappears; count drops | |
| M21 | Close Task Manager, reopen from taskbar | Reopens with a fresh list (ended tasks come back), no crash | |
| M22 | Performance tab: click Memory in the left list | Graph and stats switch to memory | |
| M24 | `cpu_color = #E53935`, `heat_low = 120,200,255`, `heat_high = #0D47A1` | CPU text/graph turn red; usage cells turn blue | |
| M23 | Garbage values: `process_count = abc`, `refresh_ms = -5`, `fluctuation = 9` | No crash; console notice; defaults/clamps used | |

## Presets (`presets/`)

Paste one at the **bottom** of `config.ini`, run, then delete it again.

- `preset_light_load.ini`: 6 processes, low CPU, slow refresh
- `preset_heavy_load.ini`: 60 processes, high load, fast refresh, opens on Performance
- `preset_custom_names.ini`: custom names, fixed seed, frozen values, no heatmap, no disk/network

## Already verified automatically (Oct 11)

All `[taskmgr]` rows M1–M24 were run by a headless test against the real `Config`, `Apps`, `Taskbar` and `config.ini`, with exact numeric checks (ranges, counts, refresh rate, seed repeatability, sort order in all 7 columns, reopen reset, 500 processes, garbage values), under AddressSanitizer with no errors. Layout was checked visually at `ui.scale` 1.0 and 2.0 and at 854×480 with a 10 px top taskbar. **Still run them once on Windows**: the real GPU build and the Desktop rows were not part of that test.
