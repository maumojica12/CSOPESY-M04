#pragma once
// ---------------------------------------------------------------------------
// Desktop: the base layer, drawn FIRST every frame.
//   - full-screen wallpaper (solid, gradient, pattern, or image)
//   - real-time clock in a configurable corner
//   - PWR button: the ONLY way to close the program
// ---------------------------------------------------------------------------
struct AppState;

namespace Desktop {
void Init(AppState& state);      // reads [desktop], [clock], [pwr] from config; loads wallpaper image
void Draw(AppState& state);      // call once per frame, before any other window
void Shutdown(AppState& state);  // frees the wallpaper texture
}
