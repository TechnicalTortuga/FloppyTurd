#include "PlatformAPI.h"
#include "AudioManager.h"
#include "ResourceManager.h"
#include "GameLog.h"
#include "AIGUI.h" // If your custom GUI functions are here, include it
#include "ResourceCompat.h"
#include "AudioClip.h"

using namespace std;

// ------------------ CONSTRUCTOR / DESTRUCTOR ------------------

AudioManager::AudioManager()
    : musicVolume(10), soundVolume(10),
    musicMuted(false), soundMuted(false)
{
    LoadTextures();
}

AudioManager::~AudioManager() {
    UnloadTextures();

    for (auto& [_, fx] : soundEffects)
        delete fx;
    soundEffects.clear();
}

// ------------------ PUBLIC METHODS ------------------

void AudioManager::StopMusic() {
    for (auto* clip : activeClips) {
        if (clip && clip->IsPlaying()) { // Only stop if clip is valid and playing
            clip->Stop();
        }
    }
}

// Clamp and set music volume
void AudioManager::SetMusicVolume(int vol) {
    musicVolume = (vol < 0) ? 0 : (vol > 10 ? 10 : vol);
}

// Clamp and set sound volume
void AudioManager::SetSoundVolume(int vol) {
    soundVolume = (vol < 0) ? 0 : (vol > 10 ? 10 : vol);
}

void AudioManager::IncreaseMusicVolume() {
    SetMusicVolume(musicVolume + 1);
}

void AudioManager::DecreaseMusicVolume() {
    SetMusicVolume(musicVolume - 1);
}

void AudioManager::IncreaseSoundVolume() {
    SetSoundVolume(soundVolume + 1);
}

void AudioManager::DecreaseSoundVolume() {
    SetSoundVolume(soundVolume - 1);
}

void AudioManager::ToggleMusicMute() {
    musicMuted = !musicMuted;
}

void AudioManager::ToggleSoundMute() {
    soundMuted = !soundMuted;
}

// ------------------ TEXTURE LOADING ------------------

void AudioManager::LoadTextures() {
    using namespace Resources;
    volumemeterEmpty = Resources::RM().GetTexture("volume_empty");
    volumemeterFull = Resources::RM().GetTexture("volume_full");
    plusButtonNormal = Resources::RM().GetTexture("plus_button");
    plusButtonHover = Resources::RM().GetTexture("plus_button_hover");
    plusButtonClicked = Resources::RM().GetTexture("plus_button");  // Fallback to normal
    minusButtonNormal = Resources::RM().GetTexture("minus_button");
    minusButtonHover = Resources::RM().GetTexture("minus_button_hover");
    minusButtonClicked = Resources::RM().GetTexture("minus_button");  // Fallback to normal
    muteButtonNormal = Resources::RM().GetTexture("mute_button");
    muteButtonHover = Resources::RM().GetTexture("mute_button_hover");
    muteButtonClicked = Resources::RM().GetTexture("mute_button");  // Fallback to normal
    muteButtonLockedNormal = Resources::RM().GetTexture("mute_button");  // Fallback to normal
    muteButtonLockedHover = Resources::RM().GetTexture("mute_button_hover");  // Fallback to hover
    muteButtonLockedClicked = Resources::RM().GetTexture("mute_button");  // Fallback to normal

    // Set nearest-neighbor filtering for UI textures
    SetTextureFilter(volumemeterEmpty, TEXTURE_FILTER_POINT);
    SetTextureFilter(volumemeterFull, TEXTURE_FILTER_POINT);
    SetTextureFilter(plusButtonNormal, TEXTURE_FILTER_POINT);
    SetTextureFilter(plusButtonHover, TEXTURE_FILTER_POINT);
    SetTextureFilter(plusButtonClicked, TEXTURE_FILTER_POINT);
    SetTextureFilter(minusButtonNormal, TEXTURE_FILTER_POINT);
    SetTextureFilter(minusButtonHover, TEXTURE_FILTER_POINT);
    SetTextureFilter(minusButtonClicked, TEXTURE_FILTER_POINT);
    SetTextureFilter(muteButtonNormal, TEXTURE_FILTER_POINT);
    SetTextureFilter(muteButtonHover, TEXTURE_FILTER_POINT);
    SetTextureFilter(muteButtonClicked, TEXTURE_FILTER_POINT);
    SetTextureFilter(muteButtonLockedNormal, TEXTURE_FILTER_POINT);
    SetTextureFilter(muteButtonLockedHover, TEXTURE_FILTER_POINT);
    SetTextureFilter(muteButtonLockedClicked, TEXTURE_FILTER_POINT);
}

void AudioManager::UnloadTextures() {
    // ResourceManager handles texture cleanup automatically
}

// ------------------ DRAWING ------------------

// Helper to handle normal/hover/clicked states
bool AudioManager::DrawButton3States(
    Texture2D normalTex,
    Texture2D hoverTex,
    Texture2D clickedTex,
    float x, float y,
    float width, float height
) {
    Vector2 mousePos = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mousePos, { x, y, width, height });
    bool pressed = hovered && IsMouseButtonDown(MOUSE_LEFT_BUTTON);
    bool released = hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);

    // Decide which texture to draw
    if (pressed) {
        DrawTexturePro(clickedTex,
            Rectangle{ 0, 0, (float)clickedTex.width, (float)clickedTex.height },
            Rectangle{ x, y, width, height },
            Vector2{ 0,0 }, 0.0f, WHITE);
    }
    else if (hovered) {
        DrawTexturePro(hoverTex,
            Rectangle{ 0, 0, (float)hoverTex.width, (float)hoverTex.height },
            Rectangle{ x, y, width, height },
            Vector2{ 0,0 }, 0.0f, WHITE);
    }
    else {
        DrawTexturePro(normalTex,
            Rectangle{ 0, 0, (float)normalTex.width, (float)normalTex.height },
            Rectangle{ x, y, width, height },
            Vector2{ 0,0 }, 0.0f, WHITE);
    }

    return released; // returns true if the mouse button was released on this button
}

void AudioManager::DrawAudioOptions(float posX, float posY)
{
    float buttonSize = 16.0f;
    float meterWidth = 60.0f;
    float rowHeight = 30.0f; // Increased for more section separation
    float labelOffsetX = 0.0f;
    float meterOffsetX = 50.0f;
    float spacing = 5.0f;
    float labelBarSpacing = 12.0f; // Increased from 8 to 12 for more padding

    // Music Row
    float currentY = posY;
    DrawTextEx(g_AIGUI.defaultFont, "Music:", Vector2{ (float)(posX + labelOffsetX), currentY }, AIGUI_FONT_SIZE_MEDIUM, 1.0f, WHITE);

    currentY += labelBarSpacing;
    float meterX = posX + meterOffsetX;
    float meterY = currentY;
    DrawTexture(volumemeterEmpty, static_cast<int>(meterX), static_cast<int>(meterY), WHITE);

    int segments = musicVolume;
    if (segments < 0) segments = 0;
    if (segments > 10) segments = 10;

    float segmentWidth = static_cast<float>(volumemeterFull.width) / 10.0f;
    Rectangle srcRect = { 0.0f, 0.0f, segmentWidth * segments, static_cast<float>(volumemeterFull.height) };
    Rectangle destRect = { meterX, meterY, segmentWidth * segments, static_cast<float>(volumemeterFull.height) };
    DrawTexturePro(volumemeterFull, srcRect, destRect, Vector2{ 0.0f, 0.0f }, 0.0f, WHITE);

    float minusX = meterX - buttonSize - spacing;
    if (AIGUI_StateButton(minusButtonNormal, minusButtonHover, minusButtonClicked, minusX, meterY, buttonSize, buttonSize))
        DecreaseMusicVolume();

    float plusX = meterX + meterWidth + spacing;
    if (AIGUI_StateButton(plusButtonNormal, plusButtonHover, plusButtonClicked, plusX, meterY, buttonSize, buttonSize))
        IncreaseMusicVolume();

    float muteX = plusX + buttonSize + spacing;
    bool useLocked = (musicMuted || musicVolume == 0);
    if (useLocked) {
        if (AIGUI_StateButton(muteButtonLockedNormal, muteButtonLockedHover, muteButtonLockedClicked, muteX, meterY, buttonSize, buttonSize))
            ToggleMusicMute();
    }
    else {
        if (AIGUI_StateButton(muteButtonNormal, muteButtonHover, muteButtonClicked, muteX, meterY, buttonSize, buttonSize))
            ToggleMusicMute();
    }

    // Sound Row
    currentY += rowHeight + spacing;
    DrawTextEx(g_AIGUI.defaultFont, "Sound:", Vector2{ (float)(posX + labelOffsetX), currentY }, AIGUI_FONT_SIZE_MEDIUM, 1.0f, WHITE);

    currentY += labelBarSpacing;
    meterX = posX + meterOffsetX;
    meterY = currentY;
    DrawTexture(volumemeterEmpty, static_cast<int>(meterX), static_cast<int>(meterY), WHITE);

    segments = soundVolume;
    if (segments < 0) segments = 0;
    if (segments > 10) segments = 10;

    segmentWidth = static_cast<float>(volumemeterFull.width) / 10.0f;
    srcRect = { 0.0f, 0.0f, segmentWidth * segments, static_cast<float>(volumemeterFull.height) };
    destRect = { meterX, meterY, segmentWidth * segments, static_cast<float>(volumemeterFull.height) };
    DrawTexturePro(volumemeterFull, srcRect, destRect, Vector2{ 0.0f, 0.0f }, 0.0f, WHITE);

    minusX = meterX - buttonSize - spacing;
    if (AIGUI_StateButton(minusButtonNormal, minusButtonHover, minusButtonClicked, minusX, meterY, buttonSize, buttonSize))
        DecreaseSoundVolume();

    plusX = meterX + meterWidth + spacing;
    if (AIGUI_StateButton(plusButtonNormal, plusButtonHover, plusButtonClicked, plusX, meterY, buttonSize, buttonSize))
        IncreaseSoundVolume();

    float muteX2 = plusX + buttonSize + spacing;
    useLocked = (soundMuted || soundVolume == 0);
    if (useLocked) {
        if (AIGUI_StateButton(muteButtonLockedNormal, muteButtonLockedHover, muteButtonLockedClicked, muteX2, meterY, buttonSize, buttonSize))
            ToggleSoundMute();
    }
    else {
        if (AIGUI_StateButton(muteButtonNormal, muteButtonHover, muteButtonClicked, muteX2, meterY, buttonSize, buttonSize))
            ToggleSoundMute();
    }
}

void AudioManager::RegisterClip(AudioClip* clip) {
    if (clip && std::find(activeClips.begin(), activeClips.end(), clip) == activeClips.end()) {
        activeClips.push_back(clip);
    }
}

void AudioManager::UnregisterClip(AudioClip* clip) {
    auto it = std::remove(activeClips.begin(), activeClips.end(), clip);
    activeClips.erase(it, activeClips.end());
}

void AudioManager::LoadSoundEffect(const std::string& name, const std::string& path) {
    if (soundEffects.count(name) == 0) {
        soundEffects[name] = new SoundEffect(path);
    }
}

void AudioManager::PlaySoundEffect(const std::string& name, float volume) {
    if (!soundMuted && soundVolume > 0 && soundEffects.count(name)) {
        float vol = (float)soundVolume / 10.0f;
        soundEffects[name]->Play(vol * volume);
    }
}