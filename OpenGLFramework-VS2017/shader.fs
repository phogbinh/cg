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

uniform vec3 viewPos;
uniform Light light;
uniform Material material;
uniform vec2 eyeOffset;
uniform sampler2D sampleTexture;

in vec3 interpolatePos;
in vec3 interpolateColor;
in vec3 interpolateNormal;
in vec2 interpolateTexCoord;

out vec4 FragColor;

void main() {
  // TODO light color
  // ambient
  vec3 ambient = light.ambient * material.ambient;
  // diffuse
  vec3 norm = normalize(interpolateNormal); // TODO interpolation may de-normalize pixel's normal vector?!
  vec3 lightDir = light.mode == 0 ? normalize(-light.direction) : normalize(light.position - interpolatePos);
  vec3 diffuse = max(dot(norm, lightDir), 0.f) * light.diffuse * material.diffuse;
  // specular
  vec3 viewDir = normalize(viewPos - interpolatePos);
  vec3 reflectDir = reflect(-lightDir, norm);
  vec3 specular = pow(max(dot(viewDir, reflectDir), 0.f), light.shininess) * light.specular * material.specular;
  // attenuation
  if (light.mode == 1 || light.mode == 2) { // point light or spot light
    float distance = length(light.position - interpolatePos);
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
  vec3 result = (ambient + diffuse + specular) * interpolateColor; // component-wise multiplication
  FragColor = texture(sampleTexture, interpolateTexCoord + eyeOffset) * vec4(result, 1.f); // component-wise multiplication
}
