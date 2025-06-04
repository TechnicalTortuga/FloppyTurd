#include "TextureCache.h"

Texture2D TextureCache::Get(const char* path)
{
    auto& m = I().map;
    auto  it = m.find(path);
    if (it != m.end()) return it->second;

    Texture2D tex = LoadTexture(path);
    SetTextureWrap(tex, TEXTURE_WRAP_CLAMP);      // never tile UI
    m[path] = tex;
    return tex;
}

void TextureCache::Clear()
{
    for (auto& kv : I().map) UnloadTexture(kv.second);
    I().map.clear();
}