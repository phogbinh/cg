#version 330 core

// [TODO] passing texture from main.cpp
// Hint: sampler2D

in vec3 interpolateColor;
in vec2 interpolateTexCoord;

out vec4 FragColor;

void main() {
  // [TODO] sampling from texture
  // Hint: texture
  FragColor = vec4(interpolateTexCoord, 0.f, 1.f);
}
