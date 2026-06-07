#pragma once

#include <glm/glm.hpp>

#include <vector>

class ObjRenderer;

struct body {
  double mass;
  glm::dvec3 pos;
  glm::dvec3 pos_err{0};
  glm::dvec3 velocity;
  glm::dvec3 vel_err{0};
  glm::dvec3 acceleration;
  int modelID;
  int objectID;
};

size_t addDynamicBody(int modelID, int objectID, double mass, glm::vec3 pos);
void updateGravity();
void updateVelocityfirst(ObjRenderer &renderer, double dt);
void updateVelocitysecond(ObjRenderer &renderer, double dt);

body &getphydata(size_t id);

double calculateTotalEnergy(std::vector<body> bodies);
