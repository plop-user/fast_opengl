
#include <SDL2/SDL_image.h>
#include <glad/glad.h>
#include <glslread.h>

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <obj.h>
#include <phy.h>
#include <print>

static std::vector<body> bodylist;

size_t addDynamicBody(int modelID, int objectID, double mass, glm::vec3 pos) {
  body temp;
  temp.mass = mass;
  temp.pos = pos;
  temp.pos_err = glm::dvec3(0);
  temp.velocity = glm::dvec3(0);
  temp.vel_err = glm::dvec3(0);
  temp.acceleration = glm::dvec3(0);
  temp.modelID = modelID;
  temp.objectID = objectID;
  bodylist.push_back(temp);

  return bodylist.size() - 1;
}

void updateGravity() {
  const double G = 1.0;
  const double softening = 0.0;

  for (auto &b : bodylist) {
    b.acceleration = glm::dvec3{0};
  }
  size_t count = bodylist.size();

  for (size_t i = 0; i < count; i++) {
    for (size_t j = i + 1; j < count; j++) {
      glm::dvec3 diff = bodylist[j].pos - bodylist[i].pos;
      double distsq = glm::dot(diff, diff) + softening;
      double invDistCube = 1.0 / (distsq * std::sqrt(distsq));

      glm::dvec3 force = diff * (G * invDistCube);

      bodylist[i].acceleration += force * bodylist[j].mass;
      bodylist[j].acceleration -= force * bodylist[i].mass;
    }
  }
}

void updateVelocityfirst(ObjRenderer &renderer, double dt) {

  for (auto &b : bodylist) {
    glm::dvec3 dv = b.acceleration * glm::dvec3(0.5 * dt) - b.vel_err;
    glm::dvec3 new_vel = b.velocity + dv;
    b.vel_err = (new_vel - b.velocity) - dv;
    b.velocity = new_vel;

    glm::dvec3 dp = b.velocity * glm::dvec3(dt) - b.pos_err;
    glm::dvec3 new_pos = b.pos + dp;
    b.pos_err = (new_pos - b.pos) - dp;
    b.pos = new_pos;
    TransformParams params;
    params.position = glm::vec3(b.pos);
    renderer.transformInstance(b.modelID, b.objectID, params);
  }
}

void updateVelocitysecond(ObjRenderer &renderer, double dt) {

  glm::dvec3 centreofmass(0.0);
  double totalMass = 0.0;

  for (auto &b : bodylist) {
    glm::dvec3 dv = b.acceleration * glm::dvec3(0.5 * dt) - b.vel_err;
    glm::dvec3 new_vel = b.velocity + dv;
    b.vel_err = (new_vel - b.velocity) - dv;
    b.velocity = new_vel;

    centreofmass += b.pos * b.mass;
    totalMass += b.mass;
  }

  centreofmass /= totalMass;
  for (auto &b : bodylist) {
    glm::dvec3 dp = -centreofmass - b.pos_err;
    glm::dvec3 new_pos = b.pos + dp;
    b.pos_err = (new_pos - b.pos) - dp;
    b.pos = new_pos;
    TransformParams params;
    params.position = glm::vec3(b.pos);
    renderer.transformInstance(b.modelID, b.objectID, params);
  }
  //	if(framec % 600 == 0){
  //	std::println("Distance:{}, x:{}, y:{}, z:{}",
  //calculateTotalEnergy(bodylist),bodylist[0].pos.x-bodylist[1].pos.x,
  //bodylist[0].pos.y-bodylist[1].pos.y, bodylist[0].pos.z-bodylist[1].pos.z);
}

body &getphydata(size_t id) { return bodylist[id]; }
