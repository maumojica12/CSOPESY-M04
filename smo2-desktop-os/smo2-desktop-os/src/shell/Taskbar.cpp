#include "shell/Taskbar.h"
#include "core/AppState.h"

// TODO (Stage 2 - Member 2):
//   1. Read [taskbar] keys from state.config in Init (position, height, colors, ...).
//   2. In Draw, render a fixed panel across the top or bottom of the main viewport
//      (flags: NoDecoration | NoMove | NoSavedSettings). It is drawn last each
//      frame, but you must still keep it above app windows when one is focused.
//   3. Every frame set state.reservedBottom (or state.reservedTop) to the panel height,
//      so the Desktop clock and PWR button move out of the way automatically.
//   4. One button per entry in state.windows: clicking toggles window.open;
//      highlight the button while window.open is true (running indicator).

void Taskbar::Init(AppState&) {}

void Taskbar::Draw(AppState&) {}
