#include "Loading.h"
#include "ResourceCompat.h"
#include "TextureCache.h"

Loading::Loading(Game* game)
	: game(game), rotationAngle(0.0f), rotationTimer(0.0f), rotationComplete(false)
{
	// Load the poophat texture via TextureCache
	poophat = Resources::RM().GetTexture("poop_hat");
	if (poophat.id == 0) {
		TraceLog(LOG_WARNING, "Failed to load PoopHat texture from cache, using fallback path");
		poophat = TextureCache::Get("resources/hats/poophat.png"); // Fallback path
	}
}

Loading::~Loading()
{
	UnloadTexture(poophat);
}

void Loading::Update()
{
	// Update rotation timer (aim for one full rotation in ~2 seconds)
	rotationTimer += GetFrameTime();
	rotationAngle = 360.0f * (rotationTimer / 2.0f);
	if (rotationTimer >= 2.0f) {
		rotationComplete = true;
	}

	// Preload MainMenu assets in the background
	if (!game->mainMenu) {
		game->mainMenu = new MainMenu(game);
	}
	if (game->mainMenu && !game->mainMenu->GetAudioClip()) {
		game->mainMenu->PlayMusic(new AudioClip("resources/mainmenu/FloppyTurdMenu.mp3"));
	}
}

void Loading::Draw()
{
	ClearBackground(BLACK);
	// Draw rotating poophat in bottom right corner, doubled to 32x32
	float x = 320.0f - 32.0f - 10.0f; // 10px margin from right edge, adjusted for 32x32
	float y = 180.0f - 32.0f - 10.0f; // 10px margin from bottom edge, adjusted for 32x32
	Rectangle dest = { x, y, 32.0f, 32.0f }; // Doubled size
	Rectangle source = { 0, 0, (float)poophat.width, (float)poophat.height };
	DrawTexturePro(poophat, source, dest, { 16.0f, 16.0f }, rotationAngle, WHITE); // Adjusted origin for center rotation
}