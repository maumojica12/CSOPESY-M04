#include "core/Texture.h"

#include <cstdio>
#include <GLFW/glfw3.h>   // also brings in the platform's OpenGL header

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F   // missing from the OpenGL 1.1 header on Windows
#endif

bool LoadTextureFromFile(const std::string& path, Texture& out) {
    int w = 0, h = 0, channels = 0;
    unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if (!pixels) {
        std::fprintf(stderr, "[Texture] Failed to load '%s': %s\n", path.c_str(), stbi_failure_reason());
        return false;
    }

    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    stbi_image_free(pixels);

    out.id = id;
    out.width = w;
    out.height = h;
    std::printf("[Texture] Loaded '%s' (%dx%d)\n", path.c_str(), w, h);
    return true;
}

void FreeTexture(Texture& tex) {
    if (tex.id) {
        GLuint id = tex.id;
        glDeleteTextures(1, &id);
    }
    tex = Texture{};
}
