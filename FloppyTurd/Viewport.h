#pragma once
#include <algorithm>
#include <cmath>
#include "RenderLayout.h"
namespace FloppyViewport {
struct View { float x, y, width, height, scale; };
inline View Fit(int width, int height, bool integerEnlargement = true,
    float logicalWidth = RenderLayout::WorldWidth, float logicalHeight = RenderLayout::WorldHeight) {
    float scale = std::max(0.001f, std::min(width / logicalWidth, height / logicalHeight));
    // Integer enlargement keeps pixel art and text evenly aligned; small embeds fit fractionally.
    if (integerEnlargement && scale >= 1) scale = std::floor(scale);
    return {(width - logicalWidth * scale) / 2, (height - logicalHeight * scale) / 2, logicalWidth * scale, logicalHeight * scale, scale};
}
}
