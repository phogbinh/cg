#version 330 core

struct Material {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform Material material;

in vec3 interpolatePos;
in vec3 interpolateColor;
in vec3 interpolateNormal;

out vec4 FragColor;

uniform float lightAmbient = 0.15f;
uniform float lightDiffuse = 1.f;
uniform float lightSpecular = 1.f;
uniform float shininess = 64.f;

void main() {
  // TODO light color
  // ambient
  vec3 ambient = lightAmbient * material.ambient;
  // diffuse
  vec3 norm = normalize(interpolateNormal); // TODO interpolation may de-normalize pixel's normal vector?!
  vec3 lightDir = normalize(lightPos - interpolatePos);
  vec3 diffuse = max(dot(norm, lightDir), 0.f) * lightDiffuse * material.diffuse;
  // specular
  vec3 viewDir = normalize(viewPos - interpolatePos);
  vec3 reflectDir = reflect(-lightDir, norm);
  vec3 specular = pow(max(dot(viewDir, reflectDir), 0.f), shininess) * lightSpecular * material.specular;
  // light
  vec3 result = (ambient + diffuse + specular) * interpolateColor; // component-wise multiplication
  FragColor = vec4(result, 1.f);
}
