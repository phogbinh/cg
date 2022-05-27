#version 330 core

uniform sampler2D sampleTexture;

in vec3 interpolateColor;
in vec2 interpolateTexCoord;

out vec4 FragColor;

void main() {
  FragColor = texture(sampleTexture, interpolateTexCoord) * vec4(interpolateColor, 1.f); // component-wise multiplication
}
