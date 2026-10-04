#include "core/Config.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

namespace {

std::string Trim(const std::string& s) {
    const size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    const size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

std::string Lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

int Clamp255(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

} // namespace

bool Config::load(const std::string& path) {
    path_ = path;
    values_.clear();

    std::ifstream in(path);
    if (!in) {
        std::fprintf(stderr, "[Config] Could not open '%s'. Using built-in defaults.\n", path.c_str());
        return false;
    }

    std::string line, section;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        line = Trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue; // comment lines

        if (line.front() == '[') {
            const size_t close = line.find(']');
            if (close == std::string::npos) {
                std::fprintf(stderr, "[Config] Line %d: missing ']' in section header.\n", lineNo);
                continue;
            }
            section = Lower(Trim(line.substr(1, close - 1)));
            continue;
        }

        const size_t eq = line.find('=');
        if (eq == std::string::npos) {
            std::fprintf(stderr, "[Config] Line %d ignored (expected key = value).\n", lineNo);
            continue;
        }

        const std::string key = Lower(Trim(line.substr(0, eq)));
        std::string value = Trim(line.substr(eq + 1));

        // Inline comment: ';' or '#' preceded by a space/tab.
        // (A leading '#' is kept, so colors like #C82828 still work.)
        for (size_t i = 1; i < value.size(); ++i) {
            if ((value[i] == ';' || value[i] == '#') && (value[i - 1] == ' ' || value[i - 1] == '\t')) {
                value = Trim(value.substr(0, i));
                break;
            }
        }

        if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
            value = value.substr(1, value.size() - 2);

        values_[section.empty() ? key : section + "." + key] = value;
    }

    std::printf("[Config] Loaded %zu values from '%s'\n", values_.size(), path.c_str());
    return true;
}

const std::string* Config::find(const std::string& key) const {
    auto it = values_.find(Lower(key));
    return it == values_.end() ? nullptr : &it->second;
}

bool Config::has(const std::string& key) const { return find(key) != nullptr; }

void Config::noteDefault(const std::string& key) const {
    if (notified_.insert(key).second)
        std::printf("[Config] '%s' not set or invalid, using default.\n", key.c_str());
}

std::string Config::getString(const std::string& key, const std::string& def) const {
    if (const std::string* v = find(key)) return *v;
    noteDefault(key);
    return def;
}

std::string Config::getChoice(const std::string& key, const std::string& def) const {
    return Lower(getString(key, def));
}

int Config::getInt(const std::string& key, int def) const {
    if (const std::string* v = find(key)) {
        try { return std::stoi(*v); } catch (...) {}
    }
    noteDefault(key);
    return def;
}

float Config::getFloat(const std::string& key, float def) const {
    if (const std::string* v = find(key)) {
        try { return std::stof(*v); } catch (...) {}
    }
    noteDefault(key);
    return def;
}

bool Config::getBool(const std::string& key, bool def) const {
    if (const std::string* v = find(key)) {
        const std::string s = Lower(*v);
        if (s == "true" || s == "1" || s == "yes" || s == "on")  return true;
        if (s == "false" || s == "0" || s == "no" || s == "off") return false;
    }
    noteDefault(key);
    return def;
}

ImU32 Config::getColor(const std::string& key, ImU32 def) const {
    const std::string* v = find(key);
    if (!v) { noteDefault(key); return def; }
    const std::string s = Trim(*v);

    try {
        if (!s.empty() && s[0] == '#') {               // #RRGGBB or #RRGGBBAA
            const std::string hex = s.substr(1);
            if (hex.size() == 6 || hex.size() == 8) {
                const unsigned long n = std::stoul(hex, nullptr, 16);
                if (hex.size() == 6)
                    return IM_COL32((n >> 16) & 0xFF, (n >> 8) & 0xFF, n & 0xFF, 255);
                return IM_COL32((n >> 24) & 0xFF, (n >> 16) & 0xFF, (n >> 8) & 0xFF, n & 0xFF);
            }
        } else {                                       // r,g,b or r,g,b,a (0-255)
            std::vector<int> parts;
            std::stringstream ss(s);
            std::string item;
            while (std::getline(ss, item, ',')) parts.push_back(std::stoi(Trim(item)));
            if (parts.size() == 3 || parts.size() == 4)
                return IM_COL32(Clamp255(parts[0]), Clamp255(parts[1]), Clamp255(parts[2]),
                                parts.size() == 4 ? Clamp255(parts[3]) : 255);
        }
    } catch (...) {}

    noteDefault(key);
    return def;
}

std::string Config::resolvePath(const std::string& relative) const {
    namespace fs = std::filesystem;
    fs::path p(relative);
    if (p.is_absolute() || path_.empty()) return p.string();
    return (fs::path(path_).parent_path() / p).string();
}
