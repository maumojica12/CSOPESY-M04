#pragma once
// ---------------------------------------------------------------------------
// Config: reads config.ini once at startup.
//
// Keys are addressed as "section.key" in lowercase, e.g. the line
//     [desktop]
//     wallpaper_mode = gradient
// is read with config.getString("desktop.wallpaper_mode", "gradient").
//
// Every getter takes a default, so a missing or misspelled key never crashes
// the program; it only prints a one-time notice in the console.
// ---------------------------------------------------------------------------
#include <string>
#include <unordered_map>
#include <unordered_set>
#include "imgui.h"

class Config {
public:
    bool load(const std::string& path);

    std::string getString(const std::string& key, const std::string& def) const;
    std::string getChoice(const std::string& key, const std::string& def) const; // lowercased string
    int         getInt   (const std::string& key, int def) const;
    float       getFloat (const std::string& key, float def) const;
    bool        getBool  (const std::string& key, bool def) const;
    ImU32       getColor (const std::string& key, ImU32 def) const; // "#RRGGBB", "#RRGGBBAA" or "r,g,b[,a]"

    bool has(const std::string& key) const;
    const std::string& path() const { return path_; }

    // Turns a path written in config.ini (e.g. assets/wallpaper.png) into a
    // path relative to the folder that contains config.ini.
    std::string resolvePath(const std::string& relative) const;

private:
    const std::string* find(const std::string& key) const;
    void noteDefault(const std::string& key) const;

    std::unordered_map<std::string, std::string> values_;
    mutable std::unordered_set<std::string> notified_;
    std::string path_;
};
