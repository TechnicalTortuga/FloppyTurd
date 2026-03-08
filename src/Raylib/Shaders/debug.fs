#version 330 core

// Debug fragment shader for wireframes and outlines (Raylib/OpenGL)
// PooperTrooper - Raylib Platform Shaders
// Created by Gnosis Engine
// Copyright © 2024 Pooper Trooper Studios. All rights reserved.

// Input from vertex shader
in vec2 fragTexCoord;
in vec4 fragColor;

// Output color
out vec4 finalColor;

// Uniforms
uniform float time;          // For animated effects
uniform vec4 debugColor;     // Override debug color

void main() {
    // Use debug color with some animation
    float pulse = 0.5 + 0.5 * sin(time * 3.0);
    finalColor = vec4(debugColor.rgb * pulse, debugColor.a);
    
    // Default to magenta if no debug color specified
    if (debugColor.a <= 0.0) {
        finalColor = vec4(1.0, 0.0, 1.0, 1.0); // Magenta
    }
}