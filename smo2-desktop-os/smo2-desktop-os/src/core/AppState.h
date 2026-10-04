#pragma once
// ---------------------------------------------------------------------------
// AppState: the single shared state of the desktop OS.
//
// Dear ImGui is immediate-mode: nothing on screen survives between frames.
// Everything that must persist (which windows are open, whether the system
// is shutting down, how much screen space the taskbar uses) lives here and
// is passed by reference to every module each frame.
// ---------------------------------------------------------------------------
#include <functional>
#include <string>
#include <vector>
#include "imgui.h"
#include "core/Config.h"

struct AppState;

// One application window (Settings, File Explorer, Task Manager, ...).
// Stage 2 registers these in apps/Apps.cpp; the Taskbar builds one button per entry.
struct AppWindow {
    std::string id;                 // unique, lowercase, no spaces (e.g. "taskmanager")
    std::string title;              // title bar text
    std::string icon;               // short label/glyph for the taskbar button
    bool        open = false;       // toggled by the taskbar, cleared by the window's [x]
    ImVec2      defaultSize = ImVec2(640.0f, 420.0f);
    std::function<void(AppState&, AppWindow&)> drawContents; // draws the BODY only
};

struct AppState {
    Config config;

    // --- System lifecycle -------------------------------------------------
    bool  running = true;           // set to false ONLY by the PWR button -> ends the main loop
    float closeBlockedTimer = 0.0f; // > 0 while the "use PWR" notice is visible

    // --- Frame timing (updated by main.cpp every frame) ---------------------
    double time = 0.0;              // seconds since start
    float  deltaTime = 0.0f;        // seconds since previous frame

    // --- Screen space claimed by the taskbar (set by Stage 2 every frame) ---
    // Desktop items (clock, PWR) are offset by these so they never sit under the taskbar.
    float reservedTop = 0.0f;
    float reservedBottom = 0.0f;

    // --- Application windows -----------------------------------------------
    std::vector<AppWindow> windows;

    AppWindow* findWindow(const std::string& id) {
        for (auto& w : windows) if (w.id == id) return &w;
        return nullptr;
    }
};
