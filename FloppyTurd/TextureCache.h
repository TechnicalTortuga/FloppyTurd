#pragma once
#include <unordered_map>
#include <string>
#include <raylib.h>

class TextureCache
{
public:
    /* lazy‑load: first request loads, later ones reuse */
    static Texture2D Get(const char* path);

    /* call once at shutdown (Playing ::~Playing shown below) */
    static void Clear();

private:
    TextureCache() = default;
    static TextureCache& I() { static TextureCache inst; return inst; }

    std::unordered_map<std::string, Texture2D> map;
};