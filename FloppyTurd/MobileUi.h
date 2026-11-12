#pragma once
#include "AIGUI.h"
#include "RenderLayout.h"
#include "PixelFont.h"
#include "AudioManager.h"
#include <string>
#include <sstream>

namespace MobileUi {
inline float ButtonHeight = 44;
inline constexpr float Gap = 10;
inline Rectangle Content() {
    ButtonHeight = RenderLayout::TouchHeight();
    auto rect = RenderLayout::SafeRect();
    rect.x += 12; rect.y += 12; rect.width -= 24; rect.height -= 24;
    return rect;
}
inline bool Button(const char* label, Rectangle rect, int fontSize = 20) {
    while (fontSize > 12 && MeasureTextEx(g_AIGUI.defaultFont,label,(float)fontSize,1).x > rect.width-12) --fontSize;
    return AIGUI_ButtonRounded(label, rect.x, rect.y, rect.width, rect.height, 0.12f, fontSize, BLACK);
}
inline void Text(Font font, const char* value, Rectangle rect, int size = 20, Color color = WHITE) {
    while (size > 12 && MeasureTextEx(font, value, (float)size, 1).x > rect.width) --size;
    DrawPixelTextCentered(font, value, rect, (float)size, 1, color);
}
inline void OutlinedText(Font font,const char* value,Rectangle rect,int size=20,Color color=WHITE) {
    for (int y=-1;y<=1;++y) for (int x=-1;x<=1;++x) {
        if (x || y) Text(font,value,{rect.x+x*1.5f,rect.y+y*1.5f,rect.width,rect.height},size,BLACK);
    }
    Text(font,value,rect,size,color);
}
inline float Wrapped(Font font, const char* value, Rectangle rect, int size = 18, Color color = WHITE) {
    std::istringstream words(value);
    std::string word, line;
    float y = rect.y;
    while (words >> word) {
        const auto candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && MeasureTextEx(font, candidate.c_str(), (float)size, 1).x > rect.width) {
            Text(font, line.c_str(), {rect.x,y,rect.width,(float)size+6}, size, color);
            y += size + 6; line = word;
        } else line = candidate;
    }
    if (!line.empty()) { Text(font, line.c_str(), {rect.x,y,rect.width,(float)size+6}, size, color); y += size + 6; }
    return y;
}
inline Rectangle Fit(Texture2D texture, Rectangle area, bool enlarge = true) {
    float scale = std::min(area.width / texture.width, area.height / texture.height);
    if (!enlarge) scale = std::min(scale, 1.0f);
    const float width = texture.width * scale, height = texture.height * scale;
    return {area.x + (area.width-width)/2, area.y + (area.height-height)/2, width, height};
}
inline void DrawAsset(Texture2D texture, Rectangle rect) {
    DrawTexturePro(texture, {0,0,(float)texture.width,(float)texture.height}, rect, {0,0}, 0, WHITE);
}
inline void Audio(Font font, Rectangle rect) {
    auto& audio = AudioManager::GetInstance();
    const float rowHeight = ButtonHeight+28;
    const bool columns = rect.width>=360 && rect.height<rowHeight*2;
    const char* names[] = {"Music", "Effects"};
    for (int row = 0; row < 2; ++row) {
        const float width = columns ? (rect.width-Gap)/2 : rect.width;
        const float x = rect.x + (columns ? row*(width+Gap) : 0);
        const float y = rect.y + (columns ? 0 : row * rowHeight);
        const int volume = row ? audio.GetSoundVolume() : audio.GetMusicVolume();
        const bool muted = row ? audio.IsSoundMuted() : audio.IsMusicMuted();
        Text(font, TextFormat("%s: %d/10", names[row], volume), {x,y,width,24}, 20);
        if (Button("-", {x,y+26,ButtonHeight,ButtonHeight})) {
            if (row) audio.DecreaseSoundVolume(); else audio.DecreaseMusicVolume();
        }
        if (Button(muted ? "Unmute" : "Mute", {x+ButtonHeight+10,y+26,width-2*(ButtonHeight+10),ButtonHeight},18)) {
            if (row) audio.ToggleSoundMute(); else audio.ToggleMusicMute();
        }
        if (Button("+", {x+width-ButtonHeight,y+26,ButtonHeight,ButtonHeight})) {
            if (row) audio.IncreaseSoundVolume(); else audio.IncreaseMusicVolume();
        }
    }
}
}
