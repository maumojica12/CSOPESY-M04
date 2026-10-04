// =============================================================================
// CSOPESY Semi-Major Output 2 - Desktop-Style OS Mock-up
// Entry point: src/main.cpp  (contains main())
//
// Stack: GLFW (window + input) -> OpenGL 3 (rendering) -> Dear ImGui (UI)
//
// Compositor render order, every frame:
//   1. Desktop  - wallpaper, clock, PWR button   (bottom layer)
//   2. Apps     - application windows             (middle layer)
//   3. Taskbar  - system panel                    (top layer)
//
// The program ends ONLY when the PWR button sets state.running = false.
// =============================================================================
#include <cstdio>
#include <filesystem>
#include <string>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include "core/AppState.h"
#include "desktop/Desktop.h"
#include "shell/Taskbar.h"
#include "apps/Apps.h"

#ifndef SMO2_PROJECT_DIR
#define SMO2_PROJECT_DIR "."
#endif

namespace {

void GlfwErrorCallback(int code, const char* description) {
    std::fprintf(stderr, "[GLFW] Error %d: %s\n", code, description);
}

// Any attempt to close the window (title-bar X, Alt+F4, taskbar "Close window")
// is cancelled here. Only the PWR button may end the program.
void BlockCloseCallback(GLFWwindow* window) {
    glfwSetWindowShouldClose(window, GLFW_FALSE);
    if (auto* state = static_cast<AppState*>(glfwGetWindowUserPointer(window)))
        state->closeBlockedTimer = 3.0f;
}

// Config lookup order:
//   1. path given as the first command-line argument
//   2. config.ini in the current working directory
//   3. config.ini in the project folder (default when running from the IDE)
std::string ResolveConfigPath(int argc, char** argv) {
    namespace fs = std::filesystem;
    if (argc > 1) return argv[1];
    if (fs::exists("config.ini")) return fs::absolute("config.ini").string();
    return (fs::path(SMO2_PROJECT_DIR) / "config.ini").string();
}

void ApplyTheme(const std::string& theme) {
    if (theme == "light")        ImGui::StyleColorsLight();
    else if (theme == "classic") ImGui::StyleColorsClassic();
    else                         ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding  = 4.0f;
    style.GrabRounding   = 4.0f;
}

} // namespace

int main(int argc, char** argv) {
    AppState state;
    state.config.load(ResolveConfigPath(argc, argv));
    const Config& cfg = state.config;

    // ------------------------------------------------------------ Window
    glfwSetErrorCallback(GlfwErrorCallback);
    if (!glfwInit()) return 1;

#if defined(__APPLE__)
    const char* glslVersion = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    const char* glslVersion = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    // window.mode: windowed | borderless | fullscreen
    const std::string mode = cfg.getChoice("window.mode", "windowed");
    int width  = cfg.getInt("window.width", 1280);
    int height = cfg.getInt("window.height", 720);
    GLFWmonitor* monitor = nullptr;

    if (mode == "borderless" || mode == "fullscreen") {
        GLFWmonitor* primary = glfwGetPrimaryMonitor();
        const GLFWvidmode* vm = glfwGetVideoMode(primary);
        width = vm->width;
        height = vm->height;
        if (mode == "fullscreen") monitor = primary;
        else                      glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    }
    glfwWindowHint(GLFW_RESIZABLE, cfg.getBool("window.resizable", false) ? GLFW_TRUE : GLFW_FALSE);

    const std::string title = cfg.getString("window.title", "CSOPESY Desktop OS");
    GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), monitor, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    if (mode == "borderless") glfwSetWindowPos(window, 0, 0);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(cfg.getBool("window.vsync", true) ? 1 : 0);

    glfwSetWindowUserPointer(window, &state);
    if (cfg.getBool("window.block_close", true))
        glfwSetWindowCloseCallback(window, BlockCloseCallback);

    // ----------------------------------------------------------- Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;                    // no imgui.ini: every run starts from the same layout
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ApplyTheme(cfg.getChoice("ui.theme", "dark"));
    const float uiScale = cfg.getFloat("ui.scale", 1.5f);   // larger text for readable recordings
    ImFontConfig fontCfg;
    fontCfg.SizePixels = 13.0f * uiScale;
    io.Fonts->AddFontDefault(&fontCfg);
    ImGui::GetStyle().ScaleAllSizes(uiScale);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);

    // ---------------------------------------------------------- OS modules
    RegisterApps(state);
    Desktop::Init(state);
    Taskbar::Init(state);
    std::printf("[System] Desktop OS started. Use the PWR button to shut down.\n");

    // ----------------------------------------------------------- Main loop
    double lastTime = glfwGetTime();
    while (state.running) {
        glfwPollEvents();

        const double now = glfwGetTime();
        state.deltaTime = static_cast<float>(now - lastTime);
        state.time = now;
        lastTime = now;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        Desktop::Draw(state);     // 1. bottom layer
        DrawAppWindows(state);    // 2. application windows
        Taskbar::Draw(state);     // 3. top layer

        ImGui::Render();
        int fbWidth = 0, fbHeight = 0;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // ------------------------------------------------------ Clean shutdown
    std::printf("[System] Shutting down...\n");
    Desktop::Shutdown(state);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("[System] Shutdown complete.\n");
    return 0;
}
