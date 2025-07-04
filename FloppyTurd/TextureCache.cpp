#include "TextureCache.h"

Texture2D TextureCache::Get(const char* path)
{
    auto& m = I().map;
    auto  it = m.find(path);
    if (it != m.end()) return it->second;

    Texture2D defaultTexture = { 0, 0, 0, 0, 0 };
    Texture2D tex = LoadTexture(path);
    if (tex.id == 0) tex = defaultTexture;
    SetTextureWrap(tex, TEXTURE_WRAP_CLAMP);      // never tile UI
    m[path] = tex;
    return tex;
}

void TextureCache::Clear()
{
    for (auto& kv : I().map) UnloadTexture(kv.second);
    I().map.clear();
}