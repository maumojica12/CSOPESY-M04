// =============================================================================
// CSOPESY SMO2 - Task Manager (Stage 3, Member 3: Raina)
//
// Window body for the "taskmanager" app registered in apps/Apps.cpp.
// The window frame, title (apps.taskmgr_title) and size (apps.taskmgr_width /
// apps.taskmgr_height) are handled by DrawAppWindows(); this file draws:
//   - Processes tab: summary header + sortable, scrollable table
//     (Name, PID, Status, CPU, Memory, Disk, Network) with heat-map cells
//   - Performance tab: CPU / Memory side list + live history graphs
//   - Dummy data generator driven by the [taskmgr] section of config.ini,
//     with slight live fluctuation every refresh interval
// =============================================================================
#pragma once

struct AppState;
struct AppWindow;

namespace TaskManager {

// Plug-in for AppWindow::drawContents.
void DrawContents(AppState& state, AppWindow& self);

// Rebuilds the dummy process list from config on the next draw.
void Reset();

} // namespace TaskManager
