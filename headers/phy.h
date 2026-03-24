#pragma once




#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <obj.h>





struct body{
	double mass;
	glm::dvec3 pos;
	glm::dvec3 velocity;
	glm::dvec3 acceleration;
	int modelID;
	int objectID;
};



size_t addDynamicBody(int modelID, int objectID, double mass, glm::vec3 pos);
void updateGravity();
void updateVelocityfirst(double dt);
void updateVelocitysecond(double dt);

body& getphydata(size_t id);


double calculateTotalEnergy(std::vector<body> bodies); 
