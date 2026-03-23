
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




struct body{
	float mass;
	glm::vec3 pos;
	glm::vec3 velocity;
	glm::vec3 acceleration;
	int modelID;
	int objectID;
};

static std::vector<body> bodylist;


size_t addDynamicBody(int modelID, int objectID, float mass, glm::vec3 pos){
	body temp;
	temp.mass = mass;
	temp.pos = pos;
	temp.velocity = glm::vec3(0);
	temp.acceleration = glm::vec3(0);
	temp.modelID = modelID;
	temp.objectID = objectID;
	bodylist.push_back(temp);
	
	return bodylist.size()-1;

}

void updateGravity(){
	const double G = 1.0;
	const double softening = 0.01;

	for(auto&b : bodylist){
		b.acceleration = glm::vec3{0.0,0.0,0.0};
	}
	size_t count = bodylist.size();
	

	for(size_t i = 0; i<count; i++){
		for(size_t j = i+1; j<count; j++){
			glm::vec3 diff = bodylist[j].pos - bodylist[i].pos;
			double distsq = glm::dot(diff, diff) + softening;
			double invDistCube = 1.0/(distsq * sqrt(distsq));

			glm::vec3 force = diff * glm::vec3(G * invDistCube);

			bodylist[i].acceleration += force * bodylist[j].mass;
			bodylist[j].acceleration -= force * bodylist[i].mass;
		}
	}
}



void updateVelocityfirst(double dt){

	for(auto&b : bodylist){
		b.velocity += b.acceleration * glm::vec3(0.5 * dt);
		b.pos += b.velocity * glm::vec3(dt);
//		b.acceleration = glm::vec3(0.0,0.0,0.0);
		objectTransform(b.modelID, b.objectID, TransformParams{.position=b.pos});

	}


}

void updateVelocitysecond(double dt){
	for(auto&b : bodylist){
		b.velocity += b.acceleration * glm::vec3(0.5 * dt);
	}
}

body& getphydata(size_t id){

	return bodylist[id];

}
