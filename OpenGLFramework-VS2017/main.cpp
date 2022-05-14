#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textfile.h"

#include "Vectors.h"
#include "Matrices.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

using namespace std;

// Default window size
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 800
const float PLANE_Y = -0.9f;

int g_windowWidth = WINDOW_WIDTH;
int g_windowHeight = WINDOW_HEIGHT;
bool mouse_pressed = false;
int starting_press_x = -1;
int starting_press_y = -1;

enum TransMode
{
  GeoTranslation = 0,
  GeoRotation = 1,
  GeoScaling = 2,
  LightEdit = 3,
  ShininessEdit = 4,
};

enum LightMode {
  Directional = 0,
  Point,
  Spot
};

struct Uniform {
  GLint iLocModelTransform;
  GLint iLocNormalTransform;
  GLint iLocMVP;
  GLint iLocViewPos;
  GLint iLocLightMode;
  GLint iLocLightPos;
  GLint iLocLightDirection;
  GLint iLocLightAmbient;
  GLint iLocLightDiffuse;
  GLint iLocLightSpecular;
  GLint iLocLightShininess;
  GLint iLocLightConstant;
  GLint iLocLightLinear;
  GLint iLocLightQuadratic;
  GLint iLocLightCosineCutOff;
  GLint iLocLightSpotExponential;
  GLint iLocMaterialAmbient;
  GLint iLocMaterialDiffuse;
  GLint iLocMaterialSpecular;
};
Uniform uniform;

vector<string> filenames; // .obj filename list

struct PhongMaterial {
  Vector3 Ka;
  Vector3 Kd;
  Vector3 Ks;
};

typedef struct {
  GLuint vao;
  GLuint vbo;
  GLuint vboTex;
  GLuint ebo;
  GLuint p_color;
  int vertex_count;
  GLuint p_normal;
  PhongMaterial material;
  int indexCount;
  GLuint m_texture;
} Shape;

struct model
{
  Vector3 position = Vector3(0, 0, 0);
  Vector3 scale = Vector3(1, 1, 1);
  Vector3 rotation = Vector3(0, 0, 0);  // Euler form
  vector<Shape> shapes;
};
vector<model> models;

struct camera
{
  Vector3 position;
  Vector3 center;
  Vector3 up_vector;
};
camera main_camera;

struct project_setting
{
  GLfloat nearClip, farClip;
  GLfloat fovy; // degree
  GLfloat aspect;
  GLfloat left, right, top, bottom;
};
project_setting proj;

GLuint phongShading;
TransMode cur_trans_mode = GeoTranslation;
LightMode g_lightMode = Directional;
Vector3 g_lightPos(1.f, 1.f, 1.f);
float g_lightDiffuse = 1.f;
float g_lightShininess = 64.f;
float g_lightCutOffDegree = 30.f;

Matrix4 view_matrix;
Matrix4 project_matrix;

int cur_idx = 0; // represent which model should be rendered now
bool g_isWireframe = false;
Matrix4 g_translation;
Matrix4 g_rotation;
Matrix4 g_scaling;

static GLvoid Normalize(GLfloat v[3])
{
  GLfloat l;

  l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
  v[0] /= l;
  v[1] /= l;
  v[2] /= l;
}

static GLvoid Cross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{

  n[0] = u[1] * v[2] - u[2] * v[1];
  n[1] = u[2] * v[0] - u[0] * v[2];
  n[2] = u[0] * v[1] - u[1] * v[0];
}


// given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
  Matrix4 mat(
  1.f, 0.f, 0.f, vec.x,
  0.f, 1.f, 0.f, vec.y,
  0.f, 0.f, 1.f, vec.z,
  0.f, 0.f, 0.f, 1.f
  );
  return mat;
}

// given a scaling vector then output a Matrix4 (Scaling Matrix)
Matrix4 scaling(Vector3 vec)
{
  Matrix4 mat(
  vec.x, 0.f,   0.f,   0.f,
  0.f,   vec.y, 0.f,   0.f,
  0.f,   0.f,   vec.z, 0.f,
  0.f,   0.f,   0.f,   1.f
  );
  return mat;
}


// given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
  Matrix4 mat(
  1.f, 0.f,      0.f,       0.f,
  0.f, cos(val), -sin(val), 0.f,
  0.f, sin(val), cos(val),  0.f,
  0.f, 0.f,      0.f,       1.f
  );
  return mat;
}

// given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
  Matrix4 mat(
  cos(val),  0.f, sin(val), 0.f,
  0.f,       1.f, 0.f,      0.f,
  -sin(val), 0.f, cos(val), 0.f,
  0.f,       0.f, 0.f,      1.f
  );
  return mat;
}

// given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
  Matrix4 mat(
  cos(val), -sin(val), 0.f, 0.f,
  sin(val), cos(val),  0.f, 0.f,
  0.f,      0.f,       1.f, 0.f,
  0.f,      0.f,       0.f, 1.f
  );
  return mat;
}

Matrix4 rotate(Vector3 vec)
{
  return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}

// compute viewing matrix accroding to the setting of main_camera
void setViewingMatrix()
{
  Vector3 eyeToCenter = main_camera.center - main_camera.position;
  Vector3 normalizedEyeToCenter = eyeToCenter; normalizedEyeToCenter.normalize();
  Vector3 firstRow = normalizedEyeToCenter.cross(main_camera.up_vector).normalize();
  Vector3 secondRow = firstRow.cross(normalizedEyeToCenter);
  Vector3 thirdRow = -normalizedEyeToCenter;
  view_matrix = Matrix4(
  firstRow.x,  firstRow.y,  firstRow.z,  0.f,
  secondRow.x, secondRow.y, secondRow.z, 0.f,
  thirdRow.x,  thirdRow.y,  thirdRow.z,  0.f,
  0.f,         0.f,         0.f,         1.f
  );
  view_matrix *= Matrix4(
  1.f, 0.f, 0.f, -main_camera.position.x,
  0.f, 1.f, 0.f, -main_camera.position.y,
  0.f, 0.f, 1.f, -main_camera.position.z,
  0.f, 0.f, 0.f, 1.f
  );
}

// compute persepective projection matrix
void setPerspective()
{
  float radian = proj.fovy / 180.f * M_PI;
  float angle = cos(radian / 2.f) / sin(radian / 2.f);
  float firstDiag  = proj.aspect >= 1.f ? angle / proj.aspect : angle;
  float secondDiag = proj.aspect >= 1.f ? angle               : angle * proj.aspect;
  project_matrix = Matrix4(
  firstDiag, 0.f,        0.f,                                                             0.f,
  0.f,       secondDiag, 0.f,                                                             0.f,
  0.f,       0.f,        (proj.farClip + proj.nearClip) / (proj.nearClip - proj.farClip), 2.f * proj.farClip * proj.nearClip / (proj.nearClip - proj.farClip),
  0.f,       0.f,        -1.f,                                                            0.f
  );
}

void setGLMatrix(GLfloat* glm, Matrix4& m) {
  glm[0] = m[0];  glm[4] = m[1];  glm[8] = m[2];   glm[12] = m[3];
  glm[1] = m[4];  glm[5] = m[5];  glm[9] = m[6];   glm[13] = m[7];
  glm[2] = m[8];  glm[6] = m[9];  glm[10] = m[10]; glm[14] = m[11];
  glm[3] = m[12]; glm[7] = m[13]; glm[11] = m[14]; glm[15] = m[15];
}

// Vertex buffers
GLuint VAO, VBO;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
  // change your aspect ratio
  g_windowWidth = width;
  g_windowHeight = height;
  proj.aspect = (float)g_windowWidth / 2.f / (float)g_windowHeight;
  setPerspective();
}

void draw(Matrix4& modelTransform, Matrix4& normalTransform, GLfloat mvp[], int x, int y) {
  // use uniform to send mvp to vertex shader
  glUniformMatrix4fv(uniform.iLocModelTransform, 1, GL_TRUE, modelTransform.get());
  glUniformMatrix4fv(uniform.iLocNormalTransform, 1, GL_TRUE, normalTransform.get());
  glUniformMatrix4fv(uniform.iLocMVP, 1, GL_FALSE, mvp);
  glUniform3f(uniform.iLocViewPos, main_camera.position.x, main_camera.position.y, main_camera.position.z);
  glUniform1i(uniform.iLocLightMode, (int)g_lightMode);
  if (g_lightMode == Directional) {
    glUniform3f(uniform.iLocLightDirection, -g_lightPos.x, -g_lightPos.y, -g_lightPos.z);
  }
  else if (g_lightMode == Point) {
    glUniform3f(uniform.iLocLightPos, g_lightPos.x, g_lightPos.y, g_lightPos.z);
  }
  else { // spot light
    glUniform3f(uniform.iLocLightPos, g_lightPos.x, g_lightPos.y, g_lightPos.z);
    glUniform3f(uniform.iLocLightDirection, 0.f, 0.f, -1.f);
    glUniform1f(uniform.iLocLightCosineCutOff, cos(g_lightCutOffDegree / 180.f * M_PI));
    glUniform1f(uniform.iLocLightSpotExponential, 50.f);
  }
  glUniform1f(uniform.iLocLightAmbient, 0.15f);
  glUniform1f(uniform.iLocLightDiffuse, g_lightDiffuse);
  glUniform1f(uniform.iLocLightSpecular, 1.f);
  glUniform1f(uniform.iLocLightShininess, g_lightShininess);
  if (g_lightMode == Point) {
    glUniform1f(uniform.iLocLightConstant,  0.01f);
    glUniform1f(uniform.iLocLightLinear,    0.8f);
    glUniform1f(uniform.iLocLightQuadratic, 0.1f);
  }
  else if (g_lightMode == Spot) {
    glUniform1f(uniform.iLocLightConstant,  0.05f);
    glUniform1f(uniform.iLocLightLinear,    0.3f);
    glUniform1f(uniform.iLocLightQuadratic, 0.6f);
  }
  for (int i = 0; i < models[cur_idx].shapes.size(); i++) 
  {
    // set glViewport and draw twice ... 
    glUniform3f(uniform.iLocMaterialAmbient,  models[cur_idx].shapes[i].material.Ka.x, models[cur_idx].shapes[i].material.Ka.y, models[cur_idx].shapes[i].material.Ka.z);
    glUniform3f(uniform.iLocMaterialDiffuse,  models[cur_idx].shapes[i].material.Kd.x, models[cur_idx].shapes[i].material.Kd.y, models[cur_idx].shapes[i].material.Kd.z);
    glUniform3f(uniform.iLocMaterialSpecular, models[cur_idx].shapes[i].material.Ks.x, models[cur_idx].shapes[i].material.Ks.y, models[cur_idx].shapes[i].material.Ks.z);
    glBindVertexArray(models[cur_idx].shapes[i].vao);
    glViewport(x, y, g_windowWidth / 2, g_windowHeight);
    glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
  }
}

// Render function for display rendering
void RenderScene(void) {  
  // clear canvas
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  Matrix4 T = translate(models[cur_idx].position);
  Matrix4 R = rotate(models[cur_idx].rotation);
  Matrix4 S = scaling(models[cur_idx].scale);
  // update translation, rotation and scaling
  g_translation = T; g_rotation = R; g_scaling = S;
  Matrix4 modelTransform = T * R * S;
  Matrix4 normalTransform(modelTransform);
  normalTransform.invert();
  normalTransform.transpose();
  Matrix4 MVP = project_matrix * view_matrix * modelTransform;
  GLfloat mvp[16];

  // row-major ---> column-major
  setGLMatrix(mvp, MVP);

  glUseProgram(phongShading);
  draw(modelTransform, normalTransform, mvp, g_windowWidth / 2, 0);
}


void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  // Call back function for keyboard
  if (key == GLFW_KEY_W && action == GLFW_PRESS) {
    g_isWireframe ^= 1;
    return;
  }
  if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
    if (cur_idx > 0) {
      --cur_idx;
    }
    else {
      cur_idx = models.size() - 1;
    }
    return;
  }
  if (key == GLFW_KEY_X && action == GLFW_PRESS) {
    if (cur_idx < models.size() - 1) {
      ++cur_idx;
    }
    else {
      cur_idx = 0;
    }
    return;
  }
  if (key == GLFW_KEY_P && action == GLFW_PRESS) {
    setPerspective();
    return;
  }
  if (key == GLFW_KEY_T && action == GLFW_PRESS) {
    cur_trans_mode = GeoTranslation;
    return;
  }
  if (key == GLFW_KEY_S && action == GLFW_PRESS) {
    cur_trans_mode = GeoScaling;
    return;
  }
  if (key == GLFW_KEY_R && action == GLFW_PRESS) {
    cur_trans_mode = GeoRotation;
    return;
  }
  if (key == GLFW_KEY_I && action == GLFW_PRESS) {
    printf("Matrix Value:\n");
    printf("Viewing Matrix:\n");
    cout << view_matrix << endl;
    printf("Projection Matrix:\n");
    cout << project_matrix << endl;
    printf("Translation Matrix:\n");
    cout << g_translation << endl;
    printf("Rotation Matrix:\n");
    cout << g_rotation << endl;
    printf("Scaling Matrix:\n");
    cout << g_scaling << endl;
    return;
  }
  if (key == GLFW_KEY_L && action == GLFW_PRESS) {
    if (g_lightMode == Directional) {
      g_lightMode = Point;
      g_lightPos = Vector3(0.f, 2.f, 1.f);
    }
    else if (g_lightMode == Point) {
      g_lightMode = Spot;
      g_lightPos = Vector3(0.f, 0.f, 2.f);
    }
    else {
      g_lightMode = Directional;
      g_lightPos = Vector3(1.f, 1.f, 1.f);
    }
    return;
  }
  if (key == GLFW_KEY_K && action == GLFW_PRESS) {
    cur_trans_mode = LightEdit;
    return;
  }
  if (key == GLFW_KEY_J && action == GLFW_PRESS) {
    cur_trans_mode = ShininessEdit;
    return;
  }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  // scroll up positive, otherwise it would be negtive
  if (cur_trans_mode == GeoTranslation) {
    models[cur_idx].position.z += yoffset;
    return;
  }
  if (cur_trans_mode == GeoScaling) {
    models[cur_idx].scale.z += yoffset / 10.f;
    return;
  }
  if (cur_trans_mode == GeoRotation) {
    models[cur_idx].rotation.z += yoffset / 5.f;
    return;
  }
  if (cur_trans_mode == LightEdit) {
    if (g_lightMode == Directional) {
      g_lightDiffuse += yoffset / 5.f;
    }
    else if (g_lightMode == Point) {
      g_lightDiffuse += yoffset / 5.f;
    }
    else { // spot light
      g_lightCutOffDegree += yoffset;
    }
    return;
  }
  if (cur_trans_mode == ShininessEdit) {
    g_lightShininess += yoffset;
    return;
  }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  // mouse press callback function
  if (action == GLFW_PRESS) {
    mouse_pressed = true;
    return;
  }
  if (action == GLFW_RELEASE) {
    mouse_pressed = false;
    starting_press_x = -1;
    starting_press_y = -1;
    return;
  }
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
  // cursor position callback function
  if (!mouse_pressed) return;
  int x = (int)xpos;
  int y = (int)ypos;
  if (starting_press_x == -1 && starting_press_y == -1) {
    starting_press_x = x;
    starting_press_y = y;
    return;
  }
  int xoffset = x - starting_press_x;
  int yoffset = starting_press_y - y;
  if (cur_trans_mode == GeoTranslation) {
    models[cur_idx].position.x += xoffset / 200.f;
    models[cur_idx].position.y += yoffset / 200.f;
  }
  else if (cur_trans_mode == GeoScaling) {
    models[cur_idx].scale.x += xoffset / 200.f;
    models[cur_idx].scale.y += yoffset / 200.f;
  }
  else if (cur_trans_mode == GeoRotation) {
    models[cur_idx].rotation.x += yoffset / 200.f;
    models[cur_idx].rotation.y -= xoffset / 200.f;
  }
  else if (cur_trans_mode == LightEdit) {
    g_lightPos.x += xoffset / 200.f;
    g_lightPos.y += yoffset / 200.f;
  }
  else {
    // intentionally empty
  }
  starting_press_x = x;
  starting_press_y = y;
}

void setShaders(GLuint& p)
{
  GLuint v, f;
  char *vs = NULL;
  char *fs = NULL;

  v = glCreateShader(GL_VERTEX_SHADER);
  f = glCreateShader(GL_FRAGMENT_SHADER);

  vs = textFileRead("shader.vs");
  fs = textFileRead("shader.fs");

  glShaderSource(v, 1, (const GLchar**)&vs, NULL);
  glShaderSource(f, 1, (const GLchar**)&fs, NULL);

  free(vs);
  free(fs);

  GLint success;
  char infoLog[1000];
  // compile vertex shader
  glCompileShader(v);
  // check for shader compile errors
  glGetShaderiv(v, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(v, 1000, NULL, infoLog);
    std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
  }

  // compile fragment shader
  glCompileShader(f);
  // check for shader compile errors
  glGetShaderiv(f, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(f, 1000, NULL, infoLog);
    std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
  }

  // create program object
  p = glCreateProgram();

  // attach shaders to program object
  glAttachShader(p,f);
  glAttachShader(p,v);

  // link program
  glLinkProgram(p);
  // check for linking errors
  glGetProgramiv(p, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(p, 1000, NULL, infoLog);
    std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
  }

  glDeleteShader(v);
  glDeleteShader(f);

  uniform.iLocModelTransform = glGetUniformLocation(p, "modelTransform");
  uniform.iLocNormalTransform = glGetUniformLocation(p, "normalTransform");
  uniform.iLocMVP = glGetUniformLocation(p, "mvp");
  uniform.iLocViewPos = glGetUniformLocation(p, "viewPos");
  uniform.iLocLightMode            = glGetUniformLocation(p, "light.mode");
  uniform.iLocLightPos             = glGetUniformLocation(p, "light.position");
  uniform.iLocLightDirection       = glGetUniformLocation(p, "light.direction");
  uniform.iLocLightAmbient         = glGetUniformLocation(p, "light.ambient");
  uniform.iLocLightDiffuse         = glGetUniformLocation(p, "light.diffuse");
  uniform.iLocLightSpecular        = glGetUniformLocation(p, "light.specular");
  uniform.iLocLightShininess       = glGetUniformLocation(p, "light.shininess");
  uniform.iLocLightConstant        = glGetUniformLocation(p, "light.constant");
  uniform.iLocLightLinear          = glGetUniformLocation(p, "light.linear");
  uniform.iLocLightQuadratic       = glGetUniformLocation(p, "light.quadratic");
  uniform.iLocLightCosineCutOff    = glGetUniformLocation(p, "light.cosineCutOff");
  uniform.iLocLightSpotExponential = glGetUniformLocation(p, "light.spotExponential");
  uniform.iLocMaterialAmbient  = glGetUniformLocation(p, "material.ambient");
  uniform.iLocMaterialDiffuse  = glGetUniformLocation(p, "material.diffuse");
  uniform.iLocMaterialSpecular = glGetUniformLocation(p, "material.specular");

  if (!success) {
    system("pause");
    exit(123);
  }
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, tinyobj::shape_t* shape)
{
  vector<float> xVector, yVector, zVector;
  float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

  // find out min and max value of X, Y and Z axis
  for (int i = 0; i < attrib->vertices.size(); i++)
  {
    //maxs = max(maxs, attrib->vertices.at(i));
    if (i % 3 == 0)
    {

      xVector.push_back(attrib->vertices.at(i));

      if (attrib->vertices.at(i) < minX)
      {
        minX = attrib->vertices.at(i);
      }

      if (attrib->vertices.at(i) > maxX)
      {
        maxX = attrib->vertices.at(i);
      }
    }
    else if (i % 3 == 1)
    {
      yVector.push_back(attrib->vertices.at(i));

      if (attrib->vertices.at(i) < minY)
      {
        minY = attrib->vertices.at(i);
      }

      if (attrib->vertices.at(i) > maxY)
      {
        maxY = attrib->vertices.at(i);
      }
    }
    else if (i % 3 == 2)
    {
      zVector.push_back(attrib->vertices.at(i));

      if (attrib->vertices.at(i) < minZ)
      {
        minZ = attrib->vertices.at(i);
      }

      if (attrib->vertices.at(i) > maxZ)
      {
        maxZ = attrib->vertices.at(i);
      }
    }
  }

  float offsetX = (maxX + minX) / 2;
  float offsetY = (maxY + minY) / 2;
  float offsetZ = (maxZ + minZ) / 2;

  for (int i = 0; i < attrib->vertices.size(); i++)
  {
    if (offsetX != 0 && i % 3 == 0)
    {
      attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
    }
    else if (offsetY != 0 && i % 3 == 1)
    {
      attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
    }
    else if (offsetZ != 0 && i % 3 == 2)
    {
      attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
    }
  }

  float greatestAxis = maxX - minX;
  float distanceOfYAxis = maxY - minY;
  float distanceOfZAxis = maxZ - minZ;

  if (distanceOfYAxis > greatestAxis)
  {
    greatestAxis = distanceOfYAxis;
  }

  if (distanceOfZAxis > greatestAxis)
  {
    greatestAxis = distanceOfZAxis;
  }

  float scale = greatestAxis / 2;

  for (int i = 0; i < attrib->vertices.size(); i++)
  {
    //std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
    attrib->vertices.at(i) = attrib->vertices.at(i) / scale;
  }
  size_t index_offset = 0;
  for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
    int fv = shape->mesh.num_face_vertices[f];

    // Loop over vertices in the face.
    for (size_t v = 0; v < fv; v++) {
      // access to vertex
      tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
      vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
      vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
      vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
      // Optional: vertex colors
      colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
      colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
      colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
      // Optional: vertex normals
      if (idx.normal_index >= 0) {
        normals.push_back(attrib->normals[3 * idx.normal_index + 0]);
        normals.push_back(attrib->normals[3 * idx.normal_index + 1]);
        normals.push_back(attrib->normals[3 * idx.normal_index + 2]);
      }
    }
    index_offset += fv;
  }
}

string GetBaseDir(const string& filepath) {
  if (filepath.find_last_of("/\\") != std::string::npos)
    return filepath.substr(0, filepath.find_last_of("/\\"));
  return "";
}

void LoadModels(string model_path)
{
  vector<tinyobj::shape_t> shapes;
  vector<tinyobj::material_t> materials;
  tinyobj::attrib_t attrib;
  vector<GLfloat> vertices;
  vector<GLfloat> colors;
  vector<GLfloat> normals;

  string err;
  string warn;

  string base_dir = GetBaseDir(model_path); // handle .mtl with relative path

#ifdef _WIN32
  base_dir += "\\";
#else
  base_dir += "/";
#endif

  bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str(), base_dir.c_str());

  if (!warn.empty()) {
    cout << warn << std::endl;
  }

  if (!err.empty()) {
    cerr << err << std::endl;
  }

  if (!ret) {
    exit(1);
  }

  printf("Load Models Success ! Shapes size %d Material size %d\n", int(shapes.size()), int(materials.size()));
  model tmp_model;

  vector<PhongMaterial> allMaterial;
  for (int i = 0; i < materials.size(); i++)
  {
    PhongMaterial material;
    material.Ka = Vector3(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
    material.Kd = Vector3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
    material.Ks = Vector3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
    allMaterial.push_back(material);
  }

  for (int i = 0; i < shapes.size(); i++)
  {

    vertices.clear();
    colors.clear();
    normals.clear();
    normalization(&attrib, vertices, colors, normals, &shapes[i]);
    // printf("Vertices size: %d", vertices.size() / 3);

    Shape tmp_shape;
    glGenVertexArrays(1, &tmp_shape.vao);
    glBindVertexArray(tmp_shape.vao);

    glGenBuffers(1, &tmp_shape.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), &vertices.at(0), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    tmp_shape.vertex_count = vertices.size() / 3;

    glGenBuffers(1, &tmp_shape.p_color);
    glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GL_FLOAT), &colors.at(0), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glGenBuffers(1, &tmp_shape.p_normal);
    glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_normal);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GL_FLOAT), &normals.at(0), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    // not support per face material, use material of first face
    if (allMaterial.size() > 0)
      tmp_shape.material = allMaterial[shapes[i].mesh.material_ids[0]];
    tmp_model.shapes.push_back(tmp_shape);
  }
  shapes.clear();
  materials.clear();
  models.push_back(tmp_model);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void initParameter()
{
  // [TODO] Setup some parameters if you need
  proj.left = -1;
  proj.right = 1;
  proj.top = 1;
  proj.bottom = -1;
  proj.nearClip = 0.001;
  proj.farClip = 100.0;
  proj.fovy = 80; // degree
  proj.aspect = (float)g_windowWidth / 2.f / (float)g_windowHeight;

  main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
  main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
  main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);

  setViewingMatrix();
  setPerspective(); //set default projection matrix as perspective matrix
}

void setupRC()
{
  // setup shaders
  setShaders(phongShading);
  initParameter();

  // OpenGL States and Values
  glClearColor(0.2, 0.2, 0.2, 1.0);
  vector<string> model_list{ "../NormalModels/bunny5KN.obj", "../NormalModels/dragon10KN.obj", "../NormalModels/lucy25KN.obj", "../NormalModels/teapot4KN.obj", "../NormalModels/dolphinN.obj"};
  // Load five model at here
  for (auto& modelFilePath : model_list) LoadModels(modelFilePath);
}

void glPrintContextInfo(bool printExtension)
{
  cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
  cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
  cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
  cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
  if (printExtension)
  {
    GLint numExt;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
    cout << "GL_EXTENSIONS =" << endl;
    for (GLint i = 0; i < numExt; i++)
    {
      cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
    }
  }
}


int main(int argc, char **argv)
{
    // initial glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
#endif

    
    // create window
  GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "110062421_HW2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    
    // load OpenGL function pointer
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
  // register glfw callback functions
    glfwSetKeyCallback(window, KeyCallback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, cursor_pos_callback);

    glfwSetFramebufferSizeCallback(window, ChangeSize);
  glEnable(GL_DEPTH_TEST);
  // Setup render context
  setupRC();

  // main loop
    while (!glfwWindowShouldClose(window))
    {
        // render
        RenderScene();
        
        // swap buffer from back to front
        glfwSwapBuffers(window);
        
        // Poll input event
        glfwPollEvents();
    }
  
  // just for compatibiliy purposes
  return 0;
}
