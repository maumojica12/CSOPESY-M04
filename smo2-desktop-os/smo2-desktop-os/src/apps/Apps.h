#pragma once
// ---------------------------------------------------------------------------
// Application windows: registration and per-frame drawing.
// ---------------------------------------------------------------------------
struct AppState;

void RegisterApps(AppState& state);   // called once at startup
void DrawAppWindows(AppState& state); // called every frame between Desktop and Taskbar
