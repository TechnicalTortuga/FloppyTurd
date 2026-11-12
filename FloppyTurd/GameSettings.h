#pragma once

namespace GameSettings
{
	inline float GameScale = 1;
	inline float GameWidth = 400;
	inline float GameHeight = 225;
    inline bool PortraitArena = false;
    inline constexpr float PortraitWidth = 200, PortraitHeight = 356;
    inline void SelectArena(bool portrait) {
        PortraitArena = portrait;
        GameWidth = portrait ? PortraitWidth : 400.0f;
        GameHeight = portrait ? PortraitHeight : 225.0f;
    }
	inline float ScreenTile = 16;

	inline int numGameSaves = 0;

	inline const char* GameVersion = "ALPHA BUILD 0.01";
};
