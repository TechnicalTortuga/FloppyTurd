#pragma once
#include <algorithm>
#include <cmath>
#include "RenderLayout.h"
namespace FloppyViewport {
struct View { float x, y, width, height, scale; };
inline View Fit(int width, int height) {
    float scale = std::max(0.001f, std::min(width / RenderLayout::WorldWidth, height / RenderLayout::WorldHeight));
    // Integer enlargement keeps pixel art and text evenly aligned; small embeds fit fractionally.
    if (scale >= 1) scale = std::floor(scale);
    return {(width - RenderLayout::WorldWidth * scale) / 2, (height - RenderLayout::WorldHeight * scale) / 2, RenderLayout::WorldWidth * scale, RenderLayout::WorldHeight * scale, scale};
}
}
