CSOPESY Semi-Major Output 2 - Desktop-Style OS Mock-up
=======================================================

Group members:
  - (Member 1 name)
  - (Member 2 name)
  - (Member 3 name)
  - (Member 4 name)

Entry point: src/main.cpp (contains main())

Requirements
------------
  - Visual Studio 2022 with the "Desktop development with C++" workload
    (includes CMake and Git), OR any C++17 compiler with CMake 3.20+ and Git
  - Internet connection on the FIRST build only (GLFW, Dear ImGui and
    stb_image are downloaded automatically)

How to run (Visual Studio 2022)
-------------------------------
  1. File > Open > Folder... and select this project folder.
  2. Wait for "CMake generation finished" in the Output window
     (the first time takes a minute while dependencies download).
  3. In the Startup Item dropdown on the toolbar, choose DesktopOS.exe.
  4. Press Run / Debug (F5).

How to run (command line)
-------------------------
  cmake -S . -B build
  cmake --build build --config Release
  build/Release/DesktopOS.exe        (Windows)
  ./build/DesktopOS                  (Linux/macOS)

Configuration (no recompiling)
------------------------------
  All visible settings are in config.ini in this folder. Edit it, save,
  and press Run again. Optionally pass a different file as the first
  argument: DesktopOS.exe my_test_config.ini

Quiz testing
------------
  docs/TEST_MATRIX.md lists every config change to try and what to expect.
  presets/ has ready-made Task Manager settings: paste one at the bottom
  of config.ini (later keys win), run, then remove it.

Shutting down
-------------
  Use the PWR button on the desktop. Closing the window by any other means
  is intentionally blocked.
