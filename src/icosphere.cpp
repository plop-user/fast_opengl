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
static unsigned int icospherep;
static int modelLoc;
static int viewLoc;
static glm::mat4 model;
static int projLoc;

constexpr float X = 0.525731112119133606; 
constexpr float Z = 0.850650808352039932;

const int divisions = 3;

static glm::vec3 icovertex[] = {
   {-X, 0.0, Z}, {X, 0.0, Z}, {-X, 0.0, -Z}, {X, 0.0, -Z},
   {0.0, Z, X}, {0.0, Z, -X}, {0.0, -Z, X}, {0.0, -Z, -X},
   {Z, X, 0.0}, {-Z, X, 0.0}, {Z, -X, 0.0}, {-Z, -X, 0.0}

};

static uint32_t icoindices[] = {
   0,4,1, 0,9,4, 9,5,4, 4,5,8, 4,8,1,
   8,10,1, 8,3,10, 5,3,8, 5,2,3, 2,7,3,
   7,10,3, 7,6,10, 7,11,6, 11,0,6, 0,1,6,
   6,1,10, 9,0,11, 9,11,2, 9,2,5, 7,2,11 
};

static std::vector<glm::vec3> vertices;
static std::vector<uint32_t> indices;


// for initializing before subdivision
void firstrun(){
	
	vertices.assign(std::begin(icovertex), std::end(icovertex));
for (auto& v : vertices)
	{    v = glm::normalize(v); }
	indices.assign(std::begin(icoindices), std::end(icoindices));
}

uint32_t midp(uint32_t p, uint32_t q, std::unordered_map<uint64_t, uint32_t>& cache){

	uint32_t min = std::min(p,q);
	uint32_t max = std::max(p,q);
	uint64_t t = (uint64_t(min) << 32) | max;

	auto it = cache.find(t);
	if(it!=cache.end()){
		return it->second;

	}
	glm::vec3 mid = glm::normalize(vertices[p]+vertices[q]);
	uint32_t index = static_cast<uint32_t>(vertices.size());
	vertices.push_back(mid);
	cache[t] = index;
	return index;
}

void subdivide(){
// for each division of the current vertices

	std::unordered_map<uint64_t, uint32_t> cache;
	std::vector<uint32_t> tempindices;
	for(uint32_t i = 0; i< indices.size(); i=i+3){
		uint32_t p = indices[i];
		uint32_t q = indices[i+1];
		uint32_t r = indices[i+2];

		uint a1 = midp(p,q,cache);
		uint a2 = midp(p, r, cache);
		uint a3 = midp(q,r, cache);


		tempindices.insert(tempindices.end(), {
 p,  a1, a2,   // top
    q,  a3, a1,   // left
    r,  a2, a3,   // right
    a1, a3, a2 
     });
			
		
	}
	 indices = std::move(tempindices);
}











void createicosphere(){

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
	icospherep = glCreateProgram();
	glAttachShader(icospherep, testvertex);
	glAttachShader(icospherep, fragshader);
	glLinkProgram(icospherep);

	// creating the icosphere 
	firstrun();

	for(int i = 0; i<(int)divisions; i++){
		subdivide();
	}

	unsigned int VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size()*sizeof(uint32_t), &indices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	
// std::cout << "sizeof(glm::vec3) = " << sizeof(glm::vec3) << std::endl;

	model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(2.0f,2.0f,2.0f));
	modelLoc = glGetUniformLocation(icospherep, "model");
	viewLoc  = glGetUniformLocation(icospherep, "view");
	projLoc  = glGetUniformLocation(icospherep, "projection");

}

void icospheredraw(glm::mat4 view, glm::mat4 pers, float time){


	if(time>0.001){model = glm::rotate(model, 0.1f, glm::vec3(0.0f,1.0f, 0.0f));}

	glUseProgram(icospherep);
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pers));
	
	glLineWidth(4.0f);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glLineWidth(1.0f);
}
