#version 330 core

// Vertex shader for 2D sprites (Raylib/OpenGL)
// FloppyTurd - Raylib Platform Shaders
// Created by Gnosis Engine
// Copyright © 2024 Floppy Turd Studios. All rights reserved.

// Input vertex attributes
layout (location = 0) in vec2 vertexPosition;
layout (location = 1) in vec2 vertexTexCoord;
layout (location = 2) in vec4 vertexColor;

// Output to fragment shader
out vec2 fragTexCoord;
out vec4 fragColor;

// Uniforms
uniform mat4 mvp; // Model-View-Projection matrix

void main() {
    // Transform vertex position
    gl_Position = mvp * vec4(vertexPosition, 0.0, 1.0);
    
    // Pass texture coordinates and color to fragment shader
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
}