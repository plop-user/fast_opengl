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

static unsigned int VAO, InstanceMatrixVBO, InstanceMatrixVAO, InstanceTexIndexVBO;
static unsigned int icospherep;
static int texlocation;
static int viewLoc;
static int projLoc;
static GLuint TextureArrayID;
static std::vector<glm::mat4> modelMatrices;
static std::vector<float> textureIndices;


constexpr float X = 0.525731112119133606;
constexpr float Z = 0.850650808352039932;


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


static std::vector<glm::vec4> instanceColors;
static unsigned int instanceColorVBO;

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











void initspheresystem(const std::vector<std::string>& texturePaths, int divisions){

	std::string myvertext = readFile("shaders/sphere_fn_vertex.glsl");
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
	std::string fragtext = readFile("shaders/sphere_fn_frag.glsl");
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
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

	// Attribute 0: Position (3 floats)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	// 4. Setup INSTANCE Buffers
	// --- Model Matrix (Locations 3, 4, 5, 6) ---
	glGenBuffers(1, &InstanceMatrixVBO);
	glBindBuffer(GL_ARRAY_BUFFER, InstanceMatrixVBO);
	std::size_t vec4Size = sizeof(glm::vec4);
	for (int i = 0; i < 4; i++) {
		glEnableVertexAttribArray(3 + i);
		glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(i * vec4Size));
		glVertexAttribDivisor(3 + i, 1);
	}

	// --- Color (Location 7) ---
	glGenBuffers(1, &instanceColorVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceColorVBO);
	glEnableVertexAttribArray(7);
	glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
	glVertexAttribDivisor(7, 1);

	glBindVertexArray(0);





	// std::cout << "sizeof(glm::vec3) = " << sizeof(glm::vec3) << std::endl;

	viewLoc  = glGetUniformLocation(icospherep, "view");
	projLoc  = glGetUniformLocation(icospherep, "projection");
	texlocation = glGetUniformLocation(icospherep, "textureArray");
}



int addSphere(float radius, glm::vec3 position, glm::vec4 color){
	int sphereID = modelMatrices.size();
	glm::mat4 mod = glm::mat4(1.0f);
	mod = glm::translate(mod, position);
	mod = glm::scale(mod, glm::vec3(radius));
	modelMatrices.push_back(mod);
	textureIndices.push_back(-1.0f);
	instanceColors.push_back(color);
	return sphereID;
}


int addSphere(float radius, glm::vec3 position, float textureIndex){
	int sphereID = modelMatrices.size();
	glm::mat4 mod = glm::mat4(1.0f);
	mod = glm::translate(mod, position);
	mod = glm::scale(mod, glm::vec3(radius));

	modelMatrices.push_back(mod);
	textureIndices.push_back(textureIndex);
	instanceColors.push_back(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	return sphereID;
}


void bufferInstanceDataSphere(){
	glBindBuffer(GL_ARRAY_BUFFER, InstanceMatrixVBO);
	glBufferData(GL_ARRAY_BUFFER, modelMatrices.size()*sizeof(glm::mat4), modelMatrices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, InstanceTexIndexVBO);
	glBufferData(GL_ARRAY_BUFFER, textureIndices.size() * sizeof(float), textureIndices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, instanceColorVBO);
	glBufferData(GL_ARRAY_BUFFER, instanceColors.size() * sizeof(glm::vec4), instanceColors.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
} 

void drawInstancedSpheres(glm::mat4 view, glm::mat4 pers){
	if (modelMatrices.empty()) return;


	glUseProgram(icospherep);
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pers));

		glLineWidth(4.0f);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glUniform1i(texlocation, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, TextureArrayID);


	glBindVertexArray(VAO);
	glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, modelMatrices.size());
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glLineWidth(1.0f);
}



void SphereTransform(int ID, glm::vec3 scaling, glm::vec3 position){
	if(ID<0 || ID>=modelMatrices.size()) return;

	glm::mat4 temp = glm::mat4(1.0f);
	temp = glm::translate(temp, position);
	temp = glm::scale(temp, scaling);
	modelMatrices[ID] = temp;
}

void SphereTransform(int ID, glm::vec3 scaling){
	if(ID<0 || ID>=modelMatrices.size()) return;

	glm::mat4 temp = glm::mat4(1.0f);
	temp = glm::scale(temp, scaling);
	modelMatrices[ID] = temp;
}
void setSphereColor(int id, glm::vec4 color){
	if(id<0 || id>=modelMatrices.size()) return;
	instanceColors[id] = color;
}

glm::vec3 getspherepos(int id){
	return glm::vec3(modelMatrices[id][3]);
}
glm::vec3 getspheresize(int id){
	glm::vec3 scale;
	scale.x = glm::length(glm::vec3(modelMatrices[id][0]));
	scale.y = glm::length(glm::vec3(modelMatrices[id][1]));
	scale.z = glm::length(glm::vec3(modelMatrices[id][2]));
	return scale;
}





void updatspherebyid(int id){
	if(id<0 || id>=modelMatrices.size()) return;
	glBindBuffer(GL_ARRAY_BUFFER, InstanceMatrixVBO);
	glBufferSubData(GL_ARRAY_BUFFER, id*sizeof(glm::mat4), sizeof(glm::mat4), &modelMatrices[id]);

	glBindBuffer(GL_ARRAY_BUFFER, InstanceMatrixVBO);
	glBufferSubData(GL_ARRAY_BUFFER, id*sizeof(glm::vec4), sizeof(glm::vec4), &instanceColors[id]);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void updatesphereall(){
	glBindBuffer(GL_ARRAY_BUFFER, InstanceMatrixVBO);
	glBufferData(GL_ARRAY_BUFFER, modelMatrices.size()*sizeof(glm::mat4), modelMatrices.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


