#pragma once
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include <stdlib.h>





int modelCount();




struct instance {
 glm::vec3 position;
    glm::vec3 scale;
  glm::vec3 rotation;

    bool dirty = true;
};



struct ObjModel {
  GLuint VAO, VBO, EBO;
  GLuint instanceMatrixVBO;
  GLuint instanceTexIndexVBO;

  int indexCount;
  std::vector<instance> instances;
  std::vector<glm::mat4> matrices;
  std::vector<float> texIndices;
  size_t matrixCapacity = 0;
  size_t texCapacity = 0;
};



struct TransformParams {
    std::optional<glm::vec3> scale;
    std::optional<glm::vec3> position;
    std::optional<glm::vec3> rotation;
};




void objectTransform(int modelID, int objectID, TransformParams temp);

int addObject(int modelID, glm::vec3 pos, glm::vec3 scale, float texID) ;
void createTextureArray(const std::vector<std::string>& paths) ;
void drawScene(glm::mat4 view, glm::mat4 proj) ;
void initShader() ;
void initSystem(const std::vector<std::string>& objPaths, const std::vector<std::string>& texPaths) ;
ObjModel loadOBJ(const std::string& path) ;
int objectCount(int modelID);
glm::vec3 scaleObject(int modelID, int objectID, glm::vec3 scale);
glm::vec3 setObjectRotation(int modelID, int objectID, glm::vec3 rot);
glm::vec3 translateObject(int modelID, int objectID, glm::vec3 pos);
void updateInstanceMatrices();
void uploadInstanceData();


instance& getData(int modelID, int objectID);
