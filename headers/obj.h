#pragma once
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <stdlib.h>

struct ObjModel {
  GLuint VAO, VBO, EBO;
  GLuint instanceMatrixVBO;
  GLuint instanceTexIndexVBO;

  int indexCount;

  std::vector<glm::mat4> matrices;
  std::vector<float> texIndices;

  size_t matrixCapacity = 0;
  size_t texCapacity = 0;
};




int addObject(int modelID, glm::vec3 pos, float scale, float texID) ;
void buildMDI() ;
GLuint compileShader(const std::string& path, GLenum type) ;
void createTextureArray(const std::vector<std::string>& paths) ;
void drawScene(glm::mat4 view, glm::mat4 proj) ;
void initShader() ;


void initSystem(const std::vector<std::string>& objPaths, const std::vector<std::string>& texPaths); 

ObjModel loadOBJ(const std::string& path);
void uploadInstanceData() ;
