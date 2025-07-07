#pragma once

// Bridge header: declares iOS-specific helpers that are implemented in
// RaylibCompat_iOS.mm but may be called from pure C++ (RaylibCompat.cpp).
// We avoid exposing Objective-C / Metal types so that this header can be safely
// included from C++ translation units.

#include "RaylibCompat.h"

#ifdef PLATFORM_IOS

#ifdef __cplusplus
extern "C" {
#endif

// Texture helpers ------------------------------------------------------------
Texture2D LoadTexture_iOS(const char *fileName);
void       UnloadTexture_iOS(Texture2D texture);
Texture2D LoadTextureFromImage_iOS(Image image);

void DrawTexture_iOS      (Texture2D texture, int posX, int posY, Color tint);
void DrawTextureV_iOS     (Texture2D texture, Vector2 position, Color tint);
void DrawTextureEx_iOS    (Texture2D texture, Vector2 position, float rotation, float scale, Color tint);
void DrawTextureRec_iOS   (Texture2D texture, Rectangle source, Rectangle dest, Color tint);
void DrawTexturePro_iOS   (Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint);

// Image helpers --------------------------------------------------------------
Image  LoadImage_iOS(const char *fileName);
void   UnloadImage_iOS(Image image);
Image  GenImageColor_iOS(int width, int height, Color color);
void   ImageDraw_iOS(Image *dst, Image src, Rectangle srcRec, Rectangle dstRec, Color tint);
void   ImageResize_iOS(Image *image, int newWidth, int newHeight);

// Render-texture helpers (optional)
void  *LoadRenderTexture_iOS(int width, int height);
void   BeginDrawing_iOS(void *renderTexture);
void   EndDrawing_iOS(void *renderTexture);

// Primitive helpers ----------------------------------------------------------
void DrawRectangle_iOS(int posX, int posY, int width, int height, unsigned int color);
void DrawText_iOS(const char *text, int posX, int posY, int fontSize, unsigned int color);

// iOS-specific function declarations
void DrawTexturePro_iOS(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint);
void DrawTexture_iOS(Texture2D texture, int posX, int posY, Color tint);
void DrawTextureV_iOS(Texture2D texture, Vector2 position, Color tint);
void DrawTextureRec_iOS(Texture2D texture, Rectangle source, Rectangle dest, Color tint);
void UnloadTexture_iOS(Texture2D texture);
void UnloadRenderTexture_iOS(RenderTexture2D target);
Texture2D CreateFallbackTexture(const char* fileName);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PLATFORM_IOS
