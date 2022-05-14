#version 330 core

struct Light {
  int mode;
  vec3 position;
  vec3 direction;
  float ambient;
  float diffuse;
  float specular;
  float shininess;
  float constant;
  float linear;
  float quadratic;
  float cosineCutOff;
  float spotExponential;
};

struct Material {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

uniform mat4 modelTransform;
uniform mat4 normalTransform;
uniform mat4 mvp;
uniform vec3 viewPos;
uniform Light light;
uniform Material material;

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;

out vec3 interpolateColor;

void main()
{
  gl_Position = mvp * vec4(aPos, 1.f);

  // TODO light color
  vec3 position = vec3(modelTransform * vec4(aPos, 1.f));
  vec3 normal = mat3(normalTransform) * aNormal;
  // ambient
  vec3 ambient = light.ambient * material.ambient;
  // diffuse
  vec3 norm = normalize(normal); // TODO is this necessary?
  vec3 lightDir = light.mode == 0 ? normalize(-light.direction) : normalize(light.position - position);
  vec3 diffuse = max(dot(norm, lightDir), 0.f) * light.diffuse * material.diffuse;
  // specular
  vec3 viewDir = normalize(viewPos - position);
  vec3 reflectDir = reflect(-lightDir, norm);
  vec3 specular = pow(max(dot(viewDir, reflectDir), 0.f), light.shininess) * light.specular * material.specular;
  // attenuation
  if (light.mode == 1 || light.mode == 2) { // point light or spot light
    float distance = length(light.position - position);
    float attenuation = 1.f / (light.constant + light.linear * distance + light.quadratic * distance * distance);
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
  }
  // spot
  if (light.mode == 2) { // spot light
    float cosineTheta = dot(lightDir, normalize(-light.direction));
    float spot = 0.f;
    if (cosineTheta > light.cosineCutOff) {
      spot = pow(max(cosineTheta, 0.f), light.spotExponential);
    }
    ambient  *= spot;
    diffuse  *= spot;
    specular *= spot;
  }
  // light
  interpolateColor = (ambient + diffuse + specular) * aColor; // component-wise multiplication
}

