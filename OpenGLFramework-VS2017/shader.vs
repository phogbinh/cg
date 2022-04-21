#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vertex_color;
uniform mat4 mvp;
uniform bool isColorInvert;

void main()
{
	gl_Position = mvp * vec4(aPos.x, aPos.y, aPos.z, 1.0);
  if (isColorInvert) {
    vertex_color = vec3(1.f, 1.f, 1.f) - aColor;
  }
  else {
    vertex_color = aColor;
  }
}

