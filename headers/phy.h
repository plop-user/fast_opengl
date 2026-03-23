#pragma once




#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <obj.h>





struct body{
	float mass;
	glm::vec3 pos;
	glm::vec3 velocity;
	glm::vec3 acceleration;
	int modelID;
	int objectID;
};



size_t addDynamicBody(int modelID, int objectID, float mass, glm::vec3 pos);
void updateGravity();
void updateVelocityfirst(double dt);
void updateVelocitysecond(double dt);

body& getphydata(size_t id);
