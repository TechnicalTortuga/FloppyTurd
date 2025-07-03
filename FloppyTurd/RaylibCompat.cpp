#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include "RaylibCompat.h"

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_IOS

// Global SDL state
static SDL_Window* g_window = nullptr;
static SDL_Renderer* g_renderer = nullptr;
static bool g_shouldClose = false;
static int g_targetFPS = 60;
static Uint32 g_frameStartTime = 0;
static Vector2 g_mousePosition = {0, 0};
static Vector2 g_mouseDelta = {0, 0};
static bool g_mousePressed[3] = {false, false, false};
static bool g_mouseReleased[3] = {false, false, false};
#if defined(__APPLE__) && TARGET_OS_IPHONE
static Font g_defaultFont = {nullptr, 16, 0, 16, {0}};
#else
static Font g_defaultFont = {nullptr, 16};
#endif

#if defined(__APPLE__) && TARGET_OS_IPHONE
#define FONT_PTR(font) ((TTF_Font*)(font.fontData))
#else
#define FONT_PTR(font) (font.font)
#endif

// ========== WINDOW FUNCTIONS ==========

void InitWindow(int width, int height, const char* title) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        SDL_Log("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return;
    }
    
    if (IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) == 0) {
        SDL_Log("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
    }
    
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        SDL_Log("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
    }
    
    if (TTF_Init() == -1) {
        SDL_Log("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
    }
    
    g_window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (g_window == nullptr) {
        SDL_Log("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return;
    }
    
    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (g_renderer == nullptr) {
        SDL_Log("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return;
    }
    
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
}

void CloseWindow() {
#if defined(__APPLE__) && TARGET_OS_IPHONE
    if (g_defaultFont.fontData) {
        TTF_CloseFont((TTF_Font*)g_defaultFont.fontData);
    }
#else
    if (g_defaultFont.font) {
        TTF_CloseFont(g_defaultFont.font);
    }
#endif
    
    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = nullptr;
    }
    
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
    }
    
    TTF_Quit();
    Mix_Quit();
    IMG_Quit();
    SDL_Quit();
}

bool WindowShouldClose() {
    SDL_Event e;
    g_mouseDelta = {0, 0};
    
    for (int i = 0; i < 3; i++) {
        g_mousePressed[i] = false;
        g_mouseReleased[i] = false;
    }
    
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            g_shouldClose = true;
        }
        else if (e.type == SDL_MOUSEMOTION) {
            g_mousePosition.x = (float)e.motion.x;
            g_mousePosition.y = (float)e.motion.y;
            g_mouseDelta.x = (float)e.motion.xrel;
            g_mouseDelta.y = (float)e.motion.yrel;
        }
        else if (e.type == SDL_MOUSEBUTTONDOWN) {
            if (e.button.button == SDL_BUTTON_LEFT) g_mousePressed[0] = true;
            else if (e.button.button == SDL_BUTTON_RIGHT) g_mousePressed[1] = true;
            else if (e.button.button == SDL_BUTTON_MIDDLE) g_mousePressed[2] = true;
        }
        else if (e.type == SDL_MOUSEBUTTONUP) {
            if (e.button.button == SDL_BUTTON_LEFT) g_mouseReleased[0] = true;
            else if (e.button.button == SDL_BUTTON_RIGHT) g_mouseReleased[1] = true;
            else if (e.button.button == SDL_BUTTON_MIDDLE) g_mouseReleased[2] = true;
        }
    }
    
    return g_shouldClose;
}

void SetTargetFPS(int fps) {
    g_targetFPS = fps;
}

int GetScreenWidth() {
    int width;
    SDL_GetWindowSize(g_window, &width, nullptr);
    return width;
}

int GetScreenHeight() {
    int height;
    SDL_GetWindowSize(g_window, nullptr, &height);
    return height;
}

// ========== DRAWING FUNCTIONS ==========

void BeginDrawing() {
    g_frameStartTime = SDL_GetTicks();
}

void EndDrawing() {
    SDL_RenderPresent(g_renderer);
    
    // Frame rate limiting
    Uint32 frameTime = SDL_GetTicks() - g_frameStartTime;
    Uint32 targetFrameTime = 1000 / g_targetFPS;
    if (frameTime < targetFrameTime) {
        SDL_Delay(targetFrameTime - frameTime);
    }
}

void ClearBackground(Color color) {
    SDL_SetRenderDrawColor(g_renderer, color.r, color.g, color.b, color.a);
    SDL_RenderClear(g_renderer);
}

// ========== TEXTURE FUNCTIONS ==========

Texture2D LoadTexture(const char* fileName) {
    SDL_Surface* surface = IMG_Load(fileName);
    Texture2D texture = {nullptr, 0, 0};
    
    if (surface == nullptr) {
        SDL_Log("Unable to load image %s! SDL_image Error: %s\n", fileName, IMG_GetError());
        return texture;
    }
    
    texture.texture = SDL_CreateTextureFromSurface(g_renderer, surface);
    texture.width = surface->w;
    texture.height = surface->h;
    
    SDL_FreeSurface(surface);
    
    if (texture.texture == nullptr) {
        SDL_Log("Unable to create texture from %s! SDL Error: %s\n", fileName, SDL_GetError());
    }
    
    return texture;
}

void UnloadTexture(Texture2D texture) {
    if (texture.texture) {
        SDL_DestroyTexture(texture.texture);
    }
}

Texture2D LoadTextureFromImage(Image image) {
    Texture2D texture = {nullptr, 0, 0};
    
    if (image.surface == nullptr) {
        return texture;
    }
    
    texture.texture = SDL_CreateTextureFromSurface(g_renderer, image.surface);
    texture.width = image.width;
    texture.height = image.height;
    
    return texture;
}

void DrawTexture(Texture2D texture, int posX, int posY, Color tint) {
    if (texture.texture == nullptr) return;
    
    SDL_Rect destRect = {posX, posY, texture.width, texture.height};
    SDL_SetTextureColorMod(texture.texture, tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaMod(texture.texture, tint.a);
    SDL_RenderCopy(g_renderer, texture.texture, nullptr, &destRect);
}

void DrawTextureV(Texture2D texture, Vector2 position, Color tint) {
    DrawTexture(texture, (int)position.x, (int)position.y, tint);
}

void DrawTextureEx(Texture2D texture, Vector2 position, float rotation, float scale, Color tint) {
    if (texture.texture == nullptr) return;
    
    SDL_Rect destRect = {
        (int)position.x, 
        (int)position.y, 
        (int)(texture.width * scale), 
        (int)(texture.height * scale)
    };
    
    SDL_SetTextureColorMod(texture.texture, tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaMod(texture.texture, tint.a);
    SDL_RenderCopyEx(g_renderer, texture.texture, nullptr, &destRect, rotation * 180.0 / M_PI, nullptr, SDL_FLIP_NONE);
}

void DrawTextureRec(Texture2D texture, Rectangle source, Vector2 position, Color tint) {
    if (texture.texture == nullptr) return;
    
    SDL_Rect srcRect = {(int)source.x, (int)source.y, (int)source.width, (int)source.height};
    SDL_Rect destRect = {(int)position.x, (int)position.y, (int)source.width, (int)source.height};
    
    SDL_SetTextureColorMod(texture.texture, tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaMod(texture.texture, tint.a);
    SDL_RenderCopy(g_renderer, texture.texture, &srcRect, &destRect);
}

void DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) {
    if (texture.texture == nullptr) return;
    
    SDL_Rect srcRect = {(int)source.x, (int)source.y, (int)source.width, (int)source.height};
    SDL_Rect destRect = {(int)dest.x, (int)dest.y, (int)dest.width, (int)dest.height};
    SDL_Point center = {(int)origin.x, (int)origin.y};
    
    SDL_SetTextureColorMod(texture.texture, tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaMod(texture.texture, tint.a);
    SDL_RenderCopyEx(g_renderer, texture.texture, &srcRect, &destRect, rotation * 180.0 / M_PI, &center, SDL_FLIP_NONE);
}

// ========== IMAGE FUNCTIONS ==========

Image LoadImage(const char* fileName) {
    Image image = {nullptr, 0, 0};
    image.surface = IMG_Load(fileName);
    
    if (image.surface) {
        image.width = image.surface->w;
        image.height = image.surface->h;
    }
    
    return image;
}

void UnloadImage(Image image) {
    if (image.surface) {
        SDL_FreeSurface(image.surface);
    }
}

Image GenImageColor(int width, int height, Color color) {
    Image image = {nullptr, width, height};
    
    image.surface = SDL_CreateRGBSurface(0, width, height, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    if (image.surface) {
        SDL_FillRect(image.surface, nullptr, SDL_MapRGBA(image.surface->format, color.r, color.g, color.b, color.a));
    }
    
    return image;
}

// ========== SOUND FUNCTIONS ==========

void InitAudioDevice() {
    // Already initialized in InitWindow
}

void CloseAudioDevice() {
    // Will be closed in CloseWindow
}

Sound LoadSound(const char* fileName) {
    Sound sound = {nullptr, 128};
    sound.chunk = Mix_LoadWAV(fileName);
    
    if (sound.chunk == nullptr) {
        SDL_Log("Failed to load sound effect! SDL_mixer Error: %s\n", Mix_GetError());
    }
    
    return sound;
}

void UnloadSound(Sound sound) {
    if (sound.chunk) {
        Mix_FreeChunk(sound.chunk);
    }
}

void PlaySound(Sound sound) {
    if (sound.chunk) {
        Mix_PlayChannel(-1, sound.chunk, 0);
    }
}

void SetSoundVolume(Sound sound, float volume) {
    if (sound.chunk) {
        Mix_VolumeChunk(sound.chunk, (int)(volume * MIX_MAX_VOLUME));
    }
}

// ========== INPUT FUNCTIONS ==========

Vector2 GetMousePosition() {
    return g_mousePosition;
}

Vector2 GetMouseDelta() {
    return g_mouseDelta;
}

bool IsMouseButtonPressed(int button) {
    return g_mousePressed[button];
}

bool IsMouseButtonReleased(int button) {
    return g_mouseReleased[button];
}

bool IsMouseButtonDown(int button) {
    Uint32 state = SDL_GetMouseState(nullptr, nullptr);
    switch (button) {
        case 0: return (state & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
        case 1: return (state & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
        case 2: return (state & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
        default: return false;
    }
}

// ========== TEXT FUNCTIONS ==========

Font LoadFont(const char* fileName) {
#if defined(__APPLE__) && TARGET_OS_IPHONE
    Font font = {nullptr, 16, 0, 16, {0}};
    font.fontData = TTF_OpenFont(fileName, 16);
    if (font.fontData == nullptr) {
        TraceLog(LOG_WARNING, "Failed to load font: %s", fileName);
    }
    return font;
#else
    Font font = {nullptr, 16};
    font.font = TTF_OpenFont(fileName, 16);
    if (font.font == nullptr) {
        TraceLog(LOG_WARNING, "Failed to load font: %s", fileName);
    }
    return font;
#endif
}

Font LoadFontEx(const char* fileName, int fontSize, int* codepoints, int codepointCount) {
#if defined(__APPLE__) && TARGET_OS_IPHONE
    Font font = {nullptr, fontSize, 0, fontSize, {0}};
    font.fontData = TTF_OpenFont(fileName, fontSize);
    if (font.fontData == nullptr) {
        TraceLog(LOG_WARNING, "Failed to load font: %s", fileName);
    }
    return font;
#else
    Font font = {nullptr, fontSize};
    font.font = TTF_OpenFont(fileName, fontSize);
    if (font.font == nullptr) {
        TraceLog(LOG_WARNING, "Failed to load font: %s", fileName);
    }
    return font;
#endif
}

void UnloadFont(Font font) {
#if defined(__APPLE__) && TARGET_OS_IPHONE
    if (font.fontData) {
        TTF_CloseFont((TTF_Font*)font.fontData);
    }
#else
    if (font.font) {
        TTF_CloseFont(font.font);
    }
#endif
}

Vector2 MeasureTextEx(Font font, const char* text, float fontSize, float spacing) {
    Vector2 size = {0, 0};
#if defined(__APPLE__) && TARGET_OS_IPHONE
    if (font.fontData && text) {
        int w, h;
        TTF_SizeText((TTF_Font*)font.fontData, text, &w, &h);
        float scale = fontSize / font.size;
        size.x = w * scale;
        size.y = h * scale;
    }
#else
    if (font.font && text) {
        int w, h;
        TTF_SizeText(font.font, text, &w, &h);
        float scale = fontSize / font.size;
        size.x = w * scale;
        size.y = h * scale;
    }
#endif
    return size;
}

void DrawTextEx(Font font, const char* text, Vector2 position, float fontSize, float spacing, Color tint) {
#if defined(__APPLE__) && TARGET_OS_IPHONE
    if (!font.fontData || !text) return;
    SDL_Color color = {tint.r, tint.g, tint.b, tint.a};
    SDL_Surface* textSurface = TTF_RenderText_Solid((TTF_Font*)font.fontData, text, color);
#else
    if (!font.font || !text) return;
    SDL_Color color = {tint.r, tint.g, tint.b, tint.a};
    SDL_Surface* textSurface = TTF_RenderText_Solid(font.font, text, color);
#endif
    
    if (textSurface) {
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(g_renderer, textSurface);
        
        float scale = fontSize / font.size;
        SDL_Rect destRect = {
            (int)position.x, 
            (int)position.y, 
            (int)(textSurface->w * scale), 
            (int)(textSurface->h * scale)
        };
        
        SDL_RenderCopy(g_renderer, textTexture, nullptr, &destRect);
        
        SDL_DestroyTexture(textTexture);
        SDL_FreeSurface(textSurface);
    }
}

void DrawText(const char* text, int posX, int posY, int fontSize, Color color) {
#if defined(__APPLE__) && TARGET_OS_IPHONE
    if (!g_defaultFont.fontData) {
        g_defaultFont.fontData = TTF_OpenFont("/System/Library/Fonts/Arial.ttf", 16);
        if (!g_defaultFont.fontData) {
            return;
        }
    }
#else
    if (!g_defaultFont.font) {
        g_defaultFont.font = TTF_OpenFont("/System/Library/Fonts/Arial.ttf", 16);
        if (!g_defaultFont.font) {
            return;
        }
    }
#endif
    DrawTextEx(g_defaultFont, text, {(float)posX, (float)posY}, (float)fontSize, 1.0f, color);
}

// ========== DRAWING PRIMITIVES ==========

void DrawRectangle(int posX, int posY, int width, int height, Color color) {
    SDL_SetRenderDrawColor(g_renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {posX, posY, width, height};
    SDL_RenderFillRect(g_renderer, &rect);
}

void DrawRectangleRec(Rectangle rec, Color color) {
    DrawRectangle((int)rec.x, (int)rec.y, (int)rec.width, (int)rec.height, color);
}

void DrawRectangleRounded(Rectangle rec, float roundness, int segments, Color color) {
    // For now, just draw a regular rectangle - rounded corners would need more complex implementation
    DrawRectangleRec(rec, color);
}

void DrawRectangleRoundedLinesEx(Rectangle rec, float roundness, int segments, float lineThick, Color color) {
    // For now, just draw rectangle outline
    SDL_SetRenderDrawColor(g_renderer, color.r, color.g, color.b, color.a);
    SDL_Rect rect = {(int)rec.x, (int)rec.y, (int)rec.width, (int)rec.height};
    
    for (int i = 0; i < (int)lineThick; i++) {
        SDL_Rect outlineRect = {rect.x - i, rect.y - i, rect.w + 2*i, rect.h + 2*i};
        SDL_RenderDrawRect(g_renderer, &outlineRect);
    }
}

// ========== MATH FUNCTIONS ==========

float Lerp(float start, float end, float amount) {
    return start + amount * (end - start);
}

Vector2 Vector2Zero() {
    return {0.0f, 0.0f};
}

Vector2 Vector2One() {
    return {1.0f, 1.0f};
}

Vector2 Vector2Add(Vector2 v1, Vector2 v2) {
    return {v1.x + v2.x, v1.y + v2.y};
}

Vector2 Vector2AddValue(Vector2 v, float add) {
    return {v.x + add, v.y + add};
}

Vector2 Vector2Subtract(Vector2 v1, Vector2 v2) {
    return {v1.x - v2.x, v1.y - v2.y};
}

Vector2 Vector2SubtractValue(Vector2 v, float sub) {
    return {v.x - sub, v.y - sub};
}

float Vector2Length(Vector2 v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

float Vector2LengthSqr(Vector2 v) {
    return v.x * v.x + v.y * v.y;
}

float Vector2DotProduct(Vector2 v1, Vector2 v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

float Vector2Distance(Vector2 v1, Vector2 v2) {
    return Vector2Length(Vector2Subtract(v1, v2));
}

float Vector2DistanceSqr(Vector2 v1, Vector2 v2) {
    return Vector2LengthSqr(Vector2Subtract(v1, v2));
}

float Vector2Angle(Vector2 v1, Vector2 v2) {
    return atan2f(v2.y - v1.y, v2.x - v1.x);
}

Vector2 Vector2Scale(Vector2 v, float scale) {
    return {v.x * scale, v.y * scale};
}

Vector2 Vector2Multiply(Vector2 v1, Vector2 v2) {
    return {v1.x * v2.x, v1.y * v2.y};
}

Vector2 Vector2Negate(Vector2 v) {
    return {-v.x, -v.y};
}

Vector2 Vector2Divide(Vector2 v1, Vector2 v2) {
    return {v1.x / v2.x, v1.y / v2.y};
}

Vector2 Vector2Normalize(Vector2 v) {
    float length = Vector2Length(v);
    if (length > 0) {
        return {v.x / length, v.y / length};
    }
    return {0, 0};
}

Vector2 Vector2Lerp(Vector2 v1, Vector2 v2, float amount) {
    return {
        Lerp(v1.x, v2.x, amount),
        Lerp(v1.y, v2.y, amount)
    };
}

Vector2 Vector2Rotate(Vector2 v, float angle) {
    float cosA = cosf(angle);
    float sinA = sinf(angle);
    return {
        v.x * cosA - v.y * sinA,
        v.x * sinA + v.y * cosA
    };
}

// ========== UTILITY FUNCTIONS ==========

bool CheckCollisionPointRec(Vector2 point, Rectangle rec) {
    return point.x >= rec.x && point.x < rec.x + rec.width &&
           point.y >= rec.y && point.y < rec.y + rec.height;
}

bool CheckCollisionRecs(Rectangle rec1, Rectangle rec2) {
    return rec1.x < rec2.x + rec2.width && rec1.x + rec1.width > rec2.x &&
           rec1.y < rec2.y + rec2.height && rec1.y + rec1.height > rec2.y;
}

// ========== TIME FUNCTIONS ==========

float GetFrameTime() {
    static Uint32 lastFrameTime = 0;
    Uint32 currentTime = SDL_GetTicks();
    float frameTime = (currentTime - lastFrameTime) / 1000.0f;
    lastFrameTime = currentTime;
    return frameTime;
}

double GetTime() {
    return SDL_GetTicks() / 1000.0;
}

void EnsureDefaultFontLoaded() {
#if defined(__APPLE__) && TARGET_OS_IPHONE
    if (!g_defaultFont.fontData) {
        g_defaultFont.fontData = TTF_OpenFont("/System/Library/Fonts/Arial.ttf", 16);
        if (!g_defaultFont.fontData) {
            TraceLog(LOG_WARNING, "Failed to load default font");
        }
    }
#else
    if (!g_defaultFont.font) {
        g_defaultFont.font = TTF_OpenFont("/System/Library/Fonts/Arial.ttf", 16);
        if (!g_defaultFont.font) {
            TraceLog(LOG_WARNING, "Failed to load default font");
        }
    }
#endif
}

#endif // TARGET_OS_IOS
#endif // __APPLE__
