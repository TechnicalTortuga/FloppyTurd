#include "EmbedInput.h"
// Credits.cpp: Implementation of the Credits scene for Floppy Turd, with scrolling text, bouncing Turdlet, and interactive FinLogo.
// Updated to use black font, add colons to position titles, integrate background scrolling with ParallaxLayer, and adjust pipe Y-position down by 20 pixels.

#include "Credits.h"
#include "ResourceCompat.h"
#include "TextureCache.h"
#include "AudioManager.h"
#include <raymath.h>
#include <random>
#include <cfloat>
#include "MobileUi.h"

Credits::Credits(Game* game)
    : textScrollOffset(320.0f + 50.0f), turdletBounce(0.0f), finLogoHovered(false), finLogoScale(1.0f), elapsedTime(0.0f), game(game)
{
    using namespace Resources;

    // Initialize CameraSystem for background scrolling
    cameraSystem = new CameraSystem();
    cameraSystem->AddLayer(new ParallaxLayer({ "resources/ui/FloppyTurdCreditsBackground.png" }, 20.0f, 1.0f)); // Slower scroll for credits

    // Load music
    music = new AudioClip(CreditsMusic);
    if (music)
    {
        // Playback starts when the credits state is entered.
        totalScrollTime = music->music.stream.buffer ? GetMusicTimeLength(music->music) : 0;
        if (totalScrollTime <= 0.0f)
        {
            TraceLog(LOG_WARNING, "Failed to get music length, falling back to 10 seconds");
            totalScrollTime = 10.0f;
        }
    }
    else
    {
        TraceLog(LOG_WARNING, "Failed to load Credits music, falling back to silence");
        delete music;
        music = nullptr;
        totalScrollTime = 10.0f;
    }

    // Initialize Turdlet sprite
    turdletSprite = std::make_shared<Sprite>("resources/turd/TurdletIdle.png", 1, 0.1f, 1.0f);
    turdletPos = { 150.0f, 50.0f };

    // Load FinLogo textures
    finLogoNormal = TextureCache::Get(FinLogo);
    finLogoHover = TextureCache::Get(FinLogo); // Placeholder
    finLogoPos = { 160.0f, 140.0f };

    // Initialize credit entries with random Y-offsets and colons
    std::default_random_engine engine{ std::random_device{}() };
    std::uniform_real_distribution<float> offsetDist(-20.0f, 20.0f);

    creditEntries = {
        // Core team
        { "Game Developer:",  "Alexandru Istrate",               offsetDist(engine) },
        { "Pixel Artist:",    "William Henson",            offsetDist(engine) },
        { "Fartist:",         "Kevin Hooks",                     offsetDist(engine) },

        { "", "", offsetDist(engine) },  // visual spacer
        // Third-party goodies
        { "Tools and Assets:", "",            offsetDist(engine) },
        { "Font (Whacky Joe):", "VEXED  � CC BY 4.0",            offsetDist(engine) },
        { "SFX Pack:",          "JD Sherbert  � Pixel UI SFX",   offsetDist(engine) },
        { "Framework:",         "Raylib 5.5 for C++", offsetDist(engine) },
        { "Music:",         "Fl Studio", offsetDist(engine) },
        { "Artwork:",         "Aseprite", offsetDist(engine) },

        { "", "", offsetDist(engine) },  // spacer
        // Special thanks
        { "Special Thanks:",   "",                                offsetDist(engine) },
        { "Betty Henson-Istrate", "Dantes - Willow",              offsetDist(engine) },

        { "", "", offsetDist(engine) },  // spacer
        // Farewell line
        { "Thank you for playing!", "",                           offsetDist(engine) }
    };

    font = game->GetScaledFont(1.0f);

    // Initialize pipes (3 pairs, centered + 20 pixels down, consistently spaced, non-random)
    float centerY = GetScreenHeight() / 2.0f + 20.0f; // Adjusted down by 20 pixels
    float startX = 320.0f;
    for (int i = 0; i < 3; ++i)
    {
        auto pipe = std::make_shared<ToiletPair>(static_cast<int>(startX), static_cast<int>(centerY), false, true);
        pipe->SetPanSpeed(80.0f);
        pipes.push_back(pipe);
        startX += 120.0f;
    }
}

Credits::~Credits()
{
    if (music) delete music;
    // The game owns the shared UI font.
    delete cameraSystem; // Clean up CameraSystem and its layers
}

void Credits::Update(float deltaTime)
{
    if (music && !music->IsPlaying()) music->Play();
    if (music && music->music.stream.buffer) {
        const float length = GetMusicTimeLength(music->music);
        if (length > 0) totalScrollTime = length;
    }
    elapsedTime += deltaTime;

    // Update text scroll offset
    textScrollOffset -= scrollSpeed * deltaTime;
    float totalWidth = 0.0f;
    for (const auto& entry : creditEntries)
    {
        if (!entry.title.empty())
            totalWidth += MeasureTextEx(font, entry.title.c_str(), 28.0f, 1.0f).x + 20.0f;
        if (!entry.name.empty())
            totalWidth += MeasureTextEx(font, entry.name.c_str(), 28.0f, 1.0f).x + 20.0f;
    }
    if (textScrollOffset < -totalWidth)
        textScrollOffset = 320.0f + 50.0f;

    // Update bouncing Turdlet
    turdletBounce += deltaTime * 2.0f;
    turdletPos.y = 50.0f + sinf(turdletBounce) * 10.0f;

    // Update pipes and maintain consistent gap
    float rightmostX = -FLT_MAX;
    for (const auto& pipe : pipes)
    {
        rightmostX = std::max(rightmostX, pipe->pos.x);
    }
    for (auto& pipe : pipes)
    {
        pipe->Update(deltaTime);
        float pipeX = pipe->pos.x;
        if (pipeX + pipe->GetTopHitbox().width < 0)
        {
            pipe->pos.x = rightmostX + 120.0f;
        }
    }

    // Update camera system (background scrolling)
    cameraSystem->Update(deltaTime);

    UpdateMusic();
}

void Credits::Draw() const
{
    if (RenderLayout::mobile) {
        using namespace MobileUi;
        const auto area=Content();
        const Rectangle list={area.x,area.y+40,area.width,area.height-100};
        Text(font,"Credits",{area.x,area.y,area.width,30},24,YELLOW);
        BeginScissorMode((int)(list.x*RenderLayout::Density),(int)(list.y*RenderLayout::Density),
            (int)(list.width*RenderLayout::Density),(int)(list.height*RenderLayout::Density));
        const float duration=std::max(10.0f,totalScrollTime);
        float y=list.y+list.height-(elapsedTime/duration)*(creditEntries.size()*82+list.height);
        for (const auto& entry : creditEntries) {
            if (!entry.title.empty()) y=Wrapped(font,entry.title.c_str(),{list.x,y,list.width,30},20,YELLOW);
            if (!entry.name.empty()) y=Wrapped(font,entry.name.c_str(),{list.x,y,list.width,30},20);
            y+=18;
        }
        EndScissorMode();
        if (Button("Back",{area.x,area.y+area.height-ButtonHeight,area.width,ButtonHeight})) {
            AudioManager::GetInstance().StopMusic();
            if (music) music->Stop();
            game->SetGameState(Game::MAINMENU);
        }
        return;
    }
    // Draw background via CameraSystem
    cameraSystem->Draw();

    // Draw pipes
    for (const auto& pipe : pipes)
    {
        pipe->Draw();
    }

    // Draw credit entries (title and name on separate lines with random offsets)
    float baseY = 90.0f;
    float xOffset = textScrollOffset;
    for (const auto& entry : creditEntries)
    {
        float yOffset = baseY + entry.yOffset;
        if (!entry.title.empty())
        {
            Vector2 textSize = MeasureTextEx(font, entry.title.c_str(), 28.0f, 1.0f);
            DrawTextEx(font, entry.title.c_str(), { xOffset, yOffset }, 28.0f, 1.0f, BLACK); // Changed to BLACK
            xOffset += textSize.x + 20.0f;
        }
        if (!entry.name.empty())
        {
            Vector2 textSize = MeasureTextEx(font, entry.name.c_str(), 28.0f, 1.0f);
            DrawTextEx(font, entry.name.c_str(), { xOffset, yOffset + 30.0f }, 28.0f, 1.0f, BLACK); // Changed to BLACK
            xOffset += textSize.x + 20.0f;
        }
    }

    // Draw bouncing Turdlet
    turdletSprite->Draw(turdletPos.x, turdletPos.y);
}

void Credits::HandleInput()
{
    if (EmbedInput::KeyPressed(KEY_ESCAPE) && game)
    {
        AudioManager::GetInstance().StopMusic();
        if (music) music->Stop();
        game->SetGameState(Game::MAINMENU);
    }
}

bool Credits::IsComplete() const
{
    return elapsedTime >= totalScrollTime;
}

void Credits::Reset()
{
    elapsedTime = 0.0f;
    textScrollOffset = 320.0f + 50.0f;
    turdletBounce = 0.0f;
    finLogoScale = 1.0f;
    finLogoHovered = false;
    if (music)
    {
        music->Stop();
        music->Play();
    }
    // Reset pipe positions
    float startX = 320.0f;
    float centerY = GetScreenHeight() / 2.0f + 20.0f; // Adjusted down by 20 pixels
    for (auto& pipe : pipes)
    {
        pipe->pos.x = startX;
        pipe->pos.y = centerY; // Ensure Y is reset to adjusted position
        startX += 120.0f;
    }
    // Reset camera system
    delete cameraSystem;
    cameraSystem = new CameraSystem();
    cameraSystem->AddLayer(new ParallaxLayer({ "resources/ui/FloppyTurdCreditsBackground.png" }, 20.0f, 1.0f));
}

void Credits::UpdateMusic()
{
    if (music)
    {
        float vol = AudioManager::GetInstance().IsMusicMuted() ? 0.0f : (float)AudioManager::GetInstance().GetMusicVolume() / 10.0f;
        music->SetVolume(vol);
        music->Update();
    }
}
