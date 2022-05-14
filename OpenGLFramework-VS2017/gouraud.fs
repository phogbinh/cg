#version 330 core

in vec3 interpolateColor;

out vec4 FragColor;

void main() {
  FragColor = vec4(interpolateColor, 1.f);
}
