#version 330 core

struct Light {
  vec3 position;
  float ambient;
  float diffuse;
  float specular;
};

struct Material {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

uniform vec3 viewPos;
uniform Light light;
uniform Material material;

in vec3 interpolatePos;
in vec3 interpolateColor;
in vec3 interpolateNormal;

out vec4 FragColor;

uniform float shininess = 64.f;

void main() {
  // TODO light color
  // ambient
  vec3 ambient = light.ambient * material.ambient;
  // diffuse
  vec3 norm = normalize(interpolateNormal); // TODO interpolation may de-normalize pixel's normal vector?!
  vec3 lightDir = normalize(light.position - interpolatePos);
  vec3 diffuse = max(dot(norm, lightDir), 0.f) * light.diffuse * material.diffuse;
  // specular
  vec3 viewDir = normalize(viewPos - interpolatePos);
  vec3 reflectDir = reflect(-lightDir, norm);
  vec3 specular = pow(max(dot(viewDir, reflectDir), 0.f), shininess) * light.specular * material.specular;
  // light
  vec3 result = (ambient + diffuse + specular) * interpolateColor; // component-wise multiplication
  FragColor = vec4(result, 1.f);
}
