#version 330 core

uniform vec2 eyeOffset;
uniform sampler2D sampleTexture;

in vec3 interpolateColor;
in vec2 interpolateTexCoord;

out vec4 FragColor;

void main() {
  FragColor = texture(sampleTexture, interpolateTexCoord + eyeOffset) * vec4(interpolateColor, 1.f); // component-wise multiplication
}
