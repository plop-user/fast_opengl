#pragma once
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
void createsphere();
void spheredraw(glm::mat4 view, glm::mat4 pers, float time);

void createicosphere();
void icospheredraw(glm::mat4 view, glm::mat4 pers, float time);

void createcube();
void cubedraw(glm::mat4 view, glm::mat4 pers, float time);

void creategrid();
void griddraw(glm::mat4 view, glm::mat4 pers);








void initcubesystem(const std::vector<std::string>& texturePaths);
int addCube(float side, glm::vec3 position, float textureIndex);
void bufferInstanceData();
void drawInstancedCubes(glm::mat4 view, glm::mat4 pers); 
int addCube(float side, glm::vec3 position, glm::vec4 color);

void SphereTransform(int ID, glm::vec3 scaling, glm::vec3 position);
void SphereTransform(int ID, glm::vec3 scaling);
int addSphere(float radius, glm::vec3 position, glm::vec4 color);
int addSphere(float radius, glm::vec3 position, float textureIndex);
void bufferInstanceDataSphere();
void drawInstancedSpheres(glm::mat4 view, glm::mat4 pers);
void firstrun();
glm::vec3 getspherepos(int id);
glm::vec3 getspheresize(int id);
void initspheresystem(const std::vector<std::string>& texturePaths, int divisions);
uint32_t midp(uint32_t p, uint32_t q, std::unordered_map<uint64_t, uint32_t>& cache);
void setSphereColor(int id, glm::vec4 color);
void subdivide();
void updatesphereall();
void updatspherebyid(int id);
void CubeTransform(int cubeID, glm::vec3 scaling, glm::vec3 position);
void CubeTransform(int cubeID, glm::vec3 scaling);
int addCube(float side, glm::vec3 position, float textureIndex) ;
void bufferInstanceData() ;
void drawInstancedCubes(glm::mat4 view, glm::mat4 pers) ;
glm::vec3 getcubepos(int id);
glm::vec3 getcubesize(int id);
void setCubeColor(int id, glm::vec4 color);
void updatecubeall();
void updatecubebyid(int id);
void rendertest();
void test();
