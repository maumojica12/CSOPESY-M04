#pragma once
#include <string>

// An OpenGL texture loaded from an image file (PNG, JPG, BMP, ...).
struct Texture {
    unsigned int id = 0;
    int width = 0;
    int height = 0;
    bool valid() const { return id != 0; }
};

bool LoadTextureFromFile(const std::string& path, Texture& out);
void FreeTexture(Texture& tex);
