#pragma once
// ---------------------------------------------------------------------------
// Taskbar: drawn LAST every frame so it stays above all application windows.
// >>> STAGE 2 (Member 2) implements this module. <<<
// ---------------------------------------------------------------------------
struct AppState;

namespace Taskbar {
void Init(AppState& state);
void Draw(AppState& state);
}
