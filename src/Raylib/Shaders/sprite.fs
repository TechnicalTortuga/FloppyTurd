#version 330 core

// Fragment shader for 2D sprites (Raylib/OpenGL)
// FloppyTurd - Raylib Platform Shaders
// Created by Gnosis Engine
// Copyright © 2024 Floppy Turd Studios. All rights reserved.

// Input from vertex shader
in vec2 fragTexCoord;
in vec4 fragColor;

// Output color
out vec4 finalColor;

// Uniforms
uniform sampler2D texture0; // Sprite texture
uniform bool useTexture;     // Whether to use texture or solid color

void main() {
    if (useTexture) {
        // Sample texture and multiply by vertex color
        vec4 texelColor = texture(texture0, fragTexCoord);
        finalColor = texelColor * fragColor;
    } else {
        // Use solid color only
        finalColor = fragColor;
    }
    
    // Discard transparent pixels
    if (finalColor.a < 0.01) {
        discard;
    }
}