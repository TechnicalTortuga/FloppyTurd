#pragma once
#include <raylib.h>
#include <cmath>
// FONT_BITMAP thresholds coverage to monochrome: point filtering alone cannot remove TTF antialiasing.
inline Font LoadPixelFont(const char* path, int size) {
    int byteCount = 0;
    unsigned char* bytes = LoadFileData(path, &byteCount);
    if (!bytes) return GetFontDefault();
    Font font{};
    font.baseSize = size;
    font.glyphPadding = 1;
    font.glyphs = LoadFontData(bytes, byteCount, size, nullptr, 95, FONT_BITMAP, &font.glyphCount);
    UnloadFileData(bytes);
    if (!font.glyphs || font.glyphCount == 0) return GetFontDefault();
    Image atlas = GenImageFontAtlas(font.glyphs, &font.recs, font.glyphCount, size, 1, 0);
    font.texture = LoadTextureFromImage(atlas);
    UnloadImage(atlas);
    SetTextureFilter(font.texture, TEXTURE_FILTER_POINT);
    return font;
}

// Align the visible glyph ink, including font bearings, rather than a guessed baseline.
inline void DrawPixelTextCentered(Font font, const char* text, Rectangle box, float size, float spacing, Color color) {
    const float scale = size / font.baseSize;
    float cursor = 0, left = 1e9f, top = 1e9f, right = -1e9f, bottom = -1e9f;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); *p; ++p) {
        const int index = GetGlyphIndex(font, *p);
        const auto& glyph = font.glyphs[index];
        const auto& rec = font.recs[index];
        if (rec.width > 0 && rec.height > 0) {
            left = fminf(left, cursor + glyph.offsetX * scale);
            top = fminf(top, glyph.offsetY * scale);
            right = fmaxf(right, cursor + (glyph.offsetX + rec.width) * scale);
            bottom = fmaxf(bottom, (glyph.offsetY + rec.height) * scale);
        }
        cursor += (glyph.advanceX ? glyph.advanceX : rec.width) * scale + spacing;
    }
    if (right < left) return;
    const Vector2 position = {roundf(box.x + (box.width - (right - left)) / 2 - left),
        roundf(box.y + (box.height - (bottom - top)) / 2 - top)};
    DrawTextEx(font, text, position, size, spacing, color);
}
