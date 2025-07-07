#pragma once

// Render layer enum for 2D layering
enum class RenderLayer {
    Background = 0,    // Background elements (sky, distant mountains, etc.)
    Midground = 1,     // Midground elements (buildings, trees, etc.)
    Foreground = 2,    // Foreground elements (player, enemies, projectiles, etc.)
    Logo = 3,          // Logo and branding elements
    UI = 4,            // UI elements (buttons, menus, etc.)
    Text = 5           // Text overlays (highest priority)
}; 