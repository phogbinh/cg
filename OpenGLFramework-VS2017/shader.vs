#version 330 core

uniform mat4 modelTransform;
uniform mat4 normalTransform;
uniform mat4 mvp;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;

out vec3 interpolatePos;
out vec3 interpolateColor;
out vec3 interpolateNormal;

void main()
{
  gl_Position = mvp * vec4(aPos, 1.f);

  interpolatePos = vec3(modelTransform * vec4(aPos, 1.f));
  interpolateColor = aColor;
  interpolateNormal = mat3(normalTransform) * aNormal;
}

