#version 330 core

uniform vec3 viewPos;
uniform vec3 lightPos;

in vec3 interpolatePos;
in vec3 interpolateColor;
in vec3 interpolateNormal;

out vec4 FragColor;

uniform float ambientIntensity = 0.15f;
uniform float specularIntensity = 1.f;
uniform float shininess = 64.f;

void main() {
  // TODO light color
  // diffuse
  vec3 norm = normalize(interpolateNormal); // TODO interpolation may de-normalize pixel's normal vector?!
  vec3 lightDir = normalize(lightPos - interpolatePos);
  float diffuse = max(dot(norm, lightDir), 0.f);
  // specular
  vec3 viewDir = normalize(viewPos - interpolatePos);
  vec3 reflectDir = reflect(-lightDir, norm);
  float specular = specularIntensity * pow(max(dot(viewDir, reflectDir), 0.f), shininess);
  // light
  vec3 result = (ambientIntensity + diffuse + specular) * interpolateColor;
  FragColor = vec4(result, 1.f);
}
