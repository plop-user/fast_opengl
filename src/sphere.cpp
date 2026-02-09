#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <iostream>
#include <string>
#include <glslread.h>
#include <cmath>
#include <numbers>

static unsigned int VAO;
static unsigned int spherep;
static int modelLoc;
static int viewLoc;
static glm::mat4 model;
static int projLoc; 
	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoords;
	};
static std::vector<unsigned int> indices;

void createsphere(){
	std::string myvertext = readFile("shaders/spherevex.glsl");
	const char* vertexfinal = myvertext.c_str();
	unsigned int testvertex = glad_glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(testvertex, 1, &vertexfinal, NULL);
	glCompileShader(testvertex);
	int success;
	char infoLog[512];
	glGetShaderiv(testvertex, GL_COMPILE_STATUS, &success);
	if (!success)
	{ 
	    glGetShaderInfoLog(testvertex, 512, NULL, infoLog);
	    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	} 


	// doing frag shader
	std::string fragtext = readFile("shaders/spherefrag.glsl");
	const char* fragtextc = fragtext.c_str();
	unsigned int fragshader = glad_glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragshader, 1, &fragtextc, NULL);
	glCompileShader(fragshader);
    glGetShaderiv(fragshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragshader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

	//linking shaders
	spherep = glCreateProgram();
	glAttachShader(spherep, testvertex);
	glAttachShader(spherep, fragshader);
	glLinkProgram(spherep);


	
	std::vector<Vertex> vertices;

	vertices.clear();
	indices.clear();


	float radius = 2.0; int sectorCount = 500; int stackCount = 500;
	
	float x, y, z, xy;
	float nx,ny, nz;
	float s,t;
	float PI = std::numbers::pi;
	float sectorStep = 2*PI/sectorCount;
	float stackStep = PI/stackCount;
	float sectorAngle, stackAngle;


	for(int i = 0; i<=stackCount; i++){
		stackAngle = PI/2 - i*stackStep;
		xy = radius*cosf(stackAngle);
		z = radius * sinf(stackAngle);
		

		for(int j = 0; j<+sectorCount; j++){
			sectorAngle = j*sectorStep;
			x = xy * cosf(sectorAngle);
			y = xy * sinf(sectorAngle);

			nx = x/radius;
			ny = y/radius;
			nz = z/radius;

			s = (float)j /sectorCount;
			t = (float)i /stackCount;
			vertices.push_back({
				glm::vec3(x,y,z),
				glm::vec3(nx,ny,nz),
				glm::vec2(s,t)
			});
		}
	}


	int k1, k2;
	for(int i = 0; i<stackCount; i++){
		k1 = i * (sectorCount + 1);
		k2 = k1 + sectorCount + 1;

		for(int j = 0; j < sectorCount; j++, k1++, k2++){
			if(i != 0){
				indices.push_back(k1);
				indices.push_back(k2);
				indices.push_back(k1 + 1);
			}
			if(i != (stackCount - 1)){
				indices.push_back(k1 + 1);
				indices.push_back(k2);
				indices.push_back(k2 + 1);
			}
		}
	}
	unsigned int VBO; unsigned int EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	model = glm::mat4(1.0f);

	modelLoc = glGetUniformLocation(spherep, "model");
	viewLoc = glGetUniformLocation(spherep, "view");
	projLoc = glGetUniformLocation(spherep, "projection");

}


void spheredraw(glm::mat4 view, glm::mat4 pers, float time){
	glUseProgram(spherep);
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pers));
	if(time >0.001f){
	model = glm::translate(model, glm::vec3(0.001f, 0.001f, 0.0f));
	}
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
}
