#include "desktop/Desktop.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>

#include "imgui.h"
#include "core/AppState.h"
#include "core/Texture.h"

namespace {

enum class Corner { TopLeft, TopRight, BottomLeft, BottomRight };

Corner ParseCorner(const std::string& s, Corner def) {
    if (s == "top_left")     return Corner::TopLeft;
    if (s == "top_right")    return Corner::TopRight;
    if (s == "bottom_left")  return Corner::BottomLeft;
    if (s == "bottom_right") return Corner::BottomRight;
    return def;
}

bool IsRight(Corner c)  { return c == Corner::TopRight || c == Corner::BottomRight; }
bool IsBottom(Corner c) { return c == Corner::BottomLeft || c == Corner::BottomRight; }

// All desktop settings, read once from config.ini in Desktop::Init.
struct Settings {
    // Wallpaper
    std::string mode;             // solid | gradient | pattern | image
    ImU32 solid = 0, top = 0, bottom = 0;
    std::string direction;        // vertical | horizontal | diagonal
    std::string patternType;      // grid | dots | stripes
    ImU32 patternColor = 0;
    float patternSpacing = 40.0f, patternThickness = 1.0f, patternSpeed = 0.0f;
    std::string imageFit;         // fill | stretch
    Texture image;

    // Clock
    bool  clockEnabled = true, clock24h = false, clockSeconds = true, clockDate = true;
    Corner clockCorner = Corner::TopRight;
    ImU32 clockColor = 0;
    float clockScale = 2.5f, margin = 16.0f;

    // PWR button
    std::string pwrLabel;
    bool  pwrConfirm = true;
    Corner pwrCorner = Corner::TopLeft;
    ImU32 pwrColor = 0;
};
Settings g;

ImU32 LerpColor(ImU32 a, ImU32 b, float t) {
    const ImVec4 x = ImGui::ColorConvertU32ToFloat4(a);
    const ImVec4 y = ImGui::ColorConvertU32ToFloat4(b);
    return ImGui::ColorConvertFloat4ToU32(ImVec4(x.x + (y.x - x.x) * t, x.y + (y.y - x.y) * t,
                                                 x.z + (y.z - x.z) * t, x.w + (y.w - x.w) * t));
}

ImU32 Shade(ImU32 c, float factor) {
    ImVec4 v = ImGui::ColorConvertU32ToFloat4(c);
    v.x = std::min(1.0f, v.x * factor);
    v.y = std::min(1.0f, v.y * factor);
    v.z = std::min(1.0f, v.z * factor);
    return ImGui::ColorConvertFloat4ToU32(v);
}

// ---------------------------------------------------------------- Wallpaper
void DrawGradient(ImDrawList* dl, ImVec2 p0, ImVec2 p1) {
    ImU32 tl, tr, br, bl;
    if (g.direction == "horizontal") {
        tl = bl = g.top;  tr = br = g.bottom;
    } else if (g.direction == "diagonal") {
        tl = g.top;  br = g.bottom;  tr = bl = LerpColor(g.top, g.bottom, 0.5f);
    } else { // vertical
        tl = tr = g.top;  bl = br = g.bottom;
    }
    dl->AddRectFilledMultiColor(p0, p1, tl, tr, br, bl);
}

void DrawPattern(ImDrawList* dl, ImVec2 p0, ImVec2 p1, double time) {
    DrawGradient(dl, p0, p1);   // pattern sits on top of the gradient

    const float s = std::max(16.0f, g.patternSpacing);
    const float off = static_cast<float>(std::fmod(time * g.patternSpeed, static_cast<double>(s)));
    const float w = p1.x - p0.x, h = p1.y - p0.y;

    if (g.patternType == "dots") {
        const float r = std::max(1.0f, g.patternThickness * 1.5f);
        for (float y = p0.y - s + off; y < p1.y + s; y += s)
            for (float x = p0.x - s + off; x < p1.x + s; x += s)
                dl->AddCircleFilled(ImVec2(x, y), r, g.patternColor, 8);
    } else if (g.patternType == "stripes") {
        for (float k = -h - s + off; k < w + s; k += s)
            dl->AddLine(ImVec2(p0.x + k, p1.y), ImVec2(p0.x + k + h, p0.y), g.patternColor, g.patternThickness);
    } else { // grid
        for (float x = p0.x - s + off; x < p1.x + s; x += s)
            dl->AddLine(ImVec2(x, p0.y), ImVec2(x, p1.y), g.patternColor, g.patternThickness);
        for (float y = p0.y - s + off; y < p1.y + s; y += s)
            dl->AddLine(ImVec2(p0.x, y), ImVec2(p1.x, y), g.patternColor, g.patternThickness);
    }
}

void DrawImage(ImDrawList* dl, ImVec2 p0, ImVec2 p1) {
    if (!g.image.valid()) { DrawGradient(dl, p0, p1); return; }   // fallback if the file failed to load

    ImVec2 uv0(0.0f, 0.0f), uv1(1.0f, 1.0f);
    if (g.imageFit == "fill") {   // keep aspect ratio, crop the overflow
        const float screenAspect = (p1.x - p0.x) / std::max(1.0f, p1.y - p0.y);
        const float imageAspect  = static_cast<float>(g.image.width) / std::max(1, g.image.height);
        if (imageAspect > screenAspect) {
            const float u = screenAspect / imageAspect;
            uv0.x = (1.0f - u) * 0.5f;  uv1.x = 1.0f - uv0.x;
        } else {
            const float v = imageAspect / screenAspect;
            uv0.y = (1.0f - v) * 0.5f;  uv1.y = 1.0f - uv0.y;
        }
    }
    dl->AddImage(static_cast<ImTextureID>(static_cast<intptr_t>(g.image.id)), p0, p1, uv0, uv1);
}

// -------------------------------------------------------------------- Clock
void FormatNow(char* timeBuf, size_t timeLen, char* dateBuf, size_t dateLen) {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    const char* fmt = g.clock24h ? (g.clockSeconds ? "%H:%M:%S" : "%H:%M")
                                 : (g.clockSeconds ? "%I:%M:%S %p" : "%I:%M %p");
    std::strftime(timeBuf, timeLen, fmt, &local);
    std::strftime(dateBuf, dateLen, "%A, %B %d, %Y", &local);
}

void DrawClock(ImDrawList* dl, ImVec2 p0, ImVec2 p1, const AppState& state) {
    char timeText[32], dateText[64];
    FormatNow(timeText, sizeof(timeText), dateText, sizeof(dateText));

    ImFont* font = ImGui::GetFont();
    const float base = ImGui::GetFontSize();
    const float bigSize = base * g.clockScale;
    const float smallSize = base * std::max(1.0f, g.clockScale * 0.45f);

    const ImVec2 timeSize = font->CalcTextSizeA(bigSize, FLT_MAX, 0.0f, timeText);
    const ImVec2 dateSize = g.clockDate ? font->CalcTextSizeA(smallSize, FLT_MAX, 0.0f, dateText) : ImVec2(0, 0);
    const float blockW = std::max(timeSize.x, dateSize.x);
    const float blockH = timeSize.y + (g.clockDate ? dateSize.y + 2.0f : 0.0f);

    const float left = IsRight(g.clockCorner) ? p1.x - g.margin - blockW : p0.x + g.margin;
    const float top  = IsBottom(g.clockCorner) ? p1.y - state.reservedBottom - g.margin - blockH
                                               : p0.y + state.reservedTop + g.margin;

    auto drawLine = [&](const char* text, float size, ImVec2 textSize, float y) {
        const float x = IsRight(g.clockCorner) ? left + blockW - textSize.x : left; // right-align on the right side
        dl->AddText(font, size, ImVec2(x + 2, y + 2), IM_COL32(0, 0, 0, 120), text);  // shadow
        dl->AddText(font, size, ImVec2(x, y), g.clockColor, text);
    };
    drawLine(timeText, bigSize, timeSize, top);
    if (g.clockDate) drawLine(dateText, smallSize, dateSize, top + timeSize.y + 2.0f);
}

// ---------------------------------------------------------------- PWR button
void DrawPowerButton(AppState& state, ImVec2 p0, ImVec2 p1) {
    const bool right = IsRight(g.pwrCorner), bottom = IsBottom(g.pwrCorner);
    const ImVec2 anchor(right ? p1.x - g.margin : p0.x + g.margin,
                        bottom ? p1.y - state.reservedBottom - g.margin : p0.y + state.reservedTop + g.margin);
    ImGui::SetNextWindowPos(anchor, ImGuiCond_Always, ImVec2(right ? 1.0f : 0.0f, bottom ? 1.0f : 0.0f));

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground |
                                   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus;   // stays on the desktop layer
    ImGui::Begin("##DesktopPowerButton", nullptr, flags);

    ImGui::PushStyleColor(ImGuiCol_Button,        g.pwrColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Shade(g.pwrColor, 1.25f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  Shade(g.pwrColor, 0.80f));
    ImGui::PushStyleColor(ImGuiCol_Text,          IM_COL32(255, 255, 255, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(16.0f, 8.0f));

    if (ImGui::Button(g.pwrLabel.c_str())) {
        if (g.pwrConfirm) ImGui::OpenPopup("Shut Down###PwrConfirm");
        else              state.running = false;     // ends the main loop -> clean shutdown in main.cpp
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Shut down");

    // Confirmation dialog, centered on screen
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Shut Down###PwrConfirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Are you sure you want to shut down?");
        ImGui::TextDisabled("All open applications will be closed.");
        ImGui::Separator();
        if (ImGui::Button("Shut Down", ImVec2(130, 0))) {
            state.running = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(130, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::End();
}

// Shown for a few seconds when the user tries to close the window any other way.
void DrawCloseBlockedNotice(AppState& state) {
    if (state.closeBlockedTimer <= 0.0f) return;
    state.closeBlockedTimer -= state.deltaTime;

    const char* msg = "Use the PWR button to shut down.";
    ImDrawList* fg = ImGui::GetForegroundDrawList();
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 size = ImGui::CalcTextSize(msg);
    const ImVec2 pad(18.0f, 10.0f);
    const ImVec2 a(vp->Pos.x + (vp->Size.x - size.x) * 0.5f - pad.x, vp->Pos.y + vp->Size.y * 0.15f);
    const ImVec2 b(a.x + size.x + pad.x * 2, a.y + size.y + pad.y * 2);
    fg->AddRectFilled(a, b, IM_COL32(20, 20, 20, 220), 8.0f);
    fg->AddText(ImVec2(a.x + pad.x, a.y + pad.y), IM_COL32(255, 255, 255, 255), msg);
}

} // namespace

// =============================================================================

void Desktop::Init(AppState& state) {
    const Config& c = state.config;

    g.mode             = c.getChoice("desktop.wallpaper_mode", "gradient");
    g.solid            = c.getColor ("desktop.solid_color", IM_COL32(24, 44, 84, 255));
    g.top              = c.getColor ("desktop.gradient_top", IM_COL32(18, 32, 72, 255));
    g.bottom           = c.getColor ("desktop.gradient_bottom", IM_COL32(64, 140, 190, 255));
    g.direction        = c.getChoice("desktop.gradient_direction", "vertical");
    g.patternType      = c.getChoice("desktop.pattern_type", "grid");
    g.patternColor     = c.getColor ("desktop.pattern_color", IM_COL32(255, 255, 255, 40));
    g.patternSpacing   = c.getFloat ("desktop.pattern_spacing", 40.0f);
    g.patternThickness = c.getFloat ("desktop.pattern_thickness", 1.0f);
    g.patternSpeed     = c.getFloat ("desktop.pattern_speed", 0.0f);
    g.imageFit         = c.getChoice("desktop.image_fit", "fill");

    g.clockEnabled = c.getBool  ("clock.enabled", true);
    g.clock24h     = c.getBool  ("clock.format_24h", false);
    g.clockSeconds = c.getBool  ("clock.show_seconds", true);
    g.clockDate    = c.getBool  ("clock.show_date", true);
    g.clockCorner  = ParseCorner(c.getChoice("clock.corner", "top_right"), Corner::TopRight);
    g.clockColor   = c.getColor ("clock.color", IM_COL32(255, 255, 255, 255));
    g.clockScale   = std::max(1.0f, c.getFloat("clock.scale", 2.5f));
    g.margin       = c.getFloat ("desktop.margin", 16.0f);

    g.pwrLabel   = c.getString("pwr.label", "PWR");
    g.pwrConfirm = c.getBool  ("pwr.confirm", true);
    g.pwrCorner  = ParseCorner(c.getChoice("pwr.corner", "top_left"), Corner::TopLeft);
    g.pwrColor   = c.getColor ("pwr.color", IM_COL32(200, 40, 40, 255));

    if (g.mode == "image") {
        const std::string path = c.resolvePath(c.getString("desktop.image_path", "assets/wallpaper.png"));
        if (!LoadTextureFromFile(path, g.image))
            std::fprintf(stderr, "[Desktop] Wallpaper image unavailable; falling back to gradient.\n");
    }
    std::printf("[Desktop] Wallpaper mode: %s\n", g.mode.c_str());
}

void Desktop::Draw(AppState& state) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 p0 = vp->Pos;
    const ImVec2 p1(vp->Pos.x + vp->Size.x, vp->Pos.y + vp->Size.y);
    ImDrawList* dl = ImGui::GetBackgroundDrawList();   // behind every ImGui window

    if      (g.mode == "solid")   dl->AddRectFilled(p0, p1, g.solid);
    else if (g.mode == "pattern") DrawPattern(dl, p0, p1, state.time);
    else if (g.mode == "image")   DrawImage(dl, p0, p1);
    else                          DrawGradient(dl, p0, p1);

    if (g.clockEnabled) DrawClock(dl, p0, p1, state);
    DrawPowerButton(state, p0, p1);
    DrawCloseBlockedNotice(state);
}

void Desktop::Shutdown(AppState&) {
    FreeTexture(g.image);
}
