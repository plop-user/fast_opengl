#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <iostream>
#include <string>
#include <glslread.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>






static unsigned int VAO, VBO, EBO, instanceMatrixVBO, instanceTexIndexVBO;
static unsigned int shaderprogram;
static int viewLoc, projLoc, texlocation;
static GLuint textureArrayID;




static std::vector<glm::mat4> modelMatrices;
static std::vector<float> textureIndices;

static std::vector<glm::vec4> instanceColors;
static unsigned int instanceColorVBO;


void initcubesystem(const std::vector<std::string>& texturePaths){
	std::string myvertext = readFile("shaders/cube_fn_vertex.glsl");
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

	std::string fragtext = readFile("shaders/cube_fn_fragment.glsl");
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
	shaderprogram = glCreateProgram();
	glAttachShader(shaderprogram, testvertex);
	glAttachShader(shaderprogram, fragshader);
	glLinkProgram(shaderprogram);


	float vertices[] = {
		// POSITIONS          // TEXTURE COORDS (Optional but common reason for 24 verts)
		// FRONT FACE (Normal: 0, 0, 1)
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // 0
		0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // 1
		0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // 2
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f, // 3

		// BACK FACE (Normal: 0, 0, -1)
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // 4
		0.5f, -0.5f, -0.5f,  1.0f, 0.0f, // 5
		0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // 6
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // 7

		// LEFT FACE (Normal: -1, 0, 0)
		-0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // 8  <-- Same position as 4, but different attributes!
		-0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // 9
		-0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // 10
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // 11

		// RIGHT FACE (Normal: 1, 0, 0)
		0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // 12
		0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // 13
		0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // 14
		0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // 15

		// TOP FACE (Normal: 0, 1, 0)
		-0.5f,  0.5f,  0.5f,  0.0f, 0.0f, // 16
		0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // 17
		0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // 18
		-0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // 19

		// BOTTOM FACE (Normal: 0, -1, 0)
		-0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // 20
		0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // 21
		0.5f, -0.5f, -0.5f,  1.0f, 1.0f, // 22
		-0.5f, -0.5f, -0.5f,  0.0f, 1.0f  // 23
	};

	unsigned int indices[] = {
		// Front
		0, 1, 2,
		2, 3, 0,

		// Back
		4, 5, 6,
		6, 7, 4,

		// Left
		8, 9, 10,
		10, 11, 8,

		// Right
		12, 13, 14,
		14, 15, 12,

		// Top
		16, 17, 18,
		18, 19, 16,

		// Bottom
		20, 21, 22,
		22, 23, 20
	};


	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));


	glGenBuffers(1, &instanceMatrixVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceMatrixVBO);

	std::size_t vec4Size = sizeof(glm::vec4);
	for (int i = 0; i < 4; i++) {
		glEnableVertexAttribArray(3 + i); 
		glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, 4 * vec4Size, (void*)(i * vec4Size));
		glVertexAttribDivisor(3 + i, 1);
	}

	glGenBuffers(1, &instanceTexIndexVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceTexIndexVBO);
	glEnableVertexAttribArray(7);
	glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
	glVertexAttribDivisor(7, 1);

	viewLoc  = glGetUniformLocation(shaderprogram, "view");
	projLoc  = glGetUniformLocation(shaderprogram, "projection");
	texlocation = glGetUniformLocation(shaderprogram, "textureArray");

	glGenBuffers(1, &instanceColorVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceColorVBO);
	glEnableVertexAttribArray(8);
	glVertexAttribPointer(8,4,GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
	glVertexAttribDivisor(8,1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	// --- Setup GL_TEXTURE_2D_ARRAY ---
	glGenTextures(1, &textureArrayID);
	glBindTexture(GL_TEXTURE_2D_ARRAY, textureArrayID);

	int layerCount = texturePaths.size();
	if (layerCount > 0) {
		// Load the first texture to establish the dimensions for the array
		SDL_Surface* firstSurface = IMG_Load(texturePaths[0].c_str());
		int width = firstSurface->w;
		int height = firstSurface->h;
		SDL_FreeSurface(firstSurface);

		// Allocate memory for the 3D texture array
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, width, height, layerCount, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

		for (int i = 0; i < layerCount; i++) {
			SDL_Surface* tempSurface = IMG_Load(texturePaths[i].c_str());
			if (!tempSurface) {
				std::cout << "Failed to load: " << texturePaths[i] << std::endl;
				continue;
			}
			SDL_Surface* formattedSurface = SDL_ConvertSurfaceFormat(tempSurface, SDL_PIXELFORMAT_RGBA32, 0);

			// Insert this image into the corresponding layer 'i'
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, formattedSurface->pixels);

			SDL_FreeSurface(tempSurface);
			SDL_FreeSurface(formattedSurface);
		}

		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
	}
}

// Add a cube's matrix and its texture index
int addCube(float side, glm::vec3 position, float textureIndex) {
	int cubeID = modelMatrices.size();
	glm::mat4 mod = glm::mat4(1.0f);
	mod = glm::translate(mod, position);
	mod = glm::scale(mod, glm::vec3(side));

	modelMatrices.push_back(mod);
	textureIndices.push_back(textureIndex);
	instanceColors.push_back(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	return cubeID;
}

int addCube(float side, glm::vec3 position, glm::vec4 color){
	int cubeID = modelMatrices.size();
	glm::mat4 mod = glm::mat4(1.0f);
	mod = glm::translate(mod, position);
	mod = glm::scale(mod, glm::vec3(side));
	modelMatrices.push_back(mod);
	textureIndices.push_back(-1.0f);
	instanceColors.push_back(color);
	return cubeID;
}

// Push data to GPU
void bufferInstanceData() {
	// Buffer Matrices
	glBindBuffer(GL_ARRAY_BUFFER, instanceMatrixVBO);
	glBufferData(GL_ARRAY_BUFFER, modelMatrices.size() * sizeof(glm::mat4), modelMatrices.data(), GL_STATIC_DRAW);

	// Buffer Texture Indices
	glBindBuffer(GL_ARRAY_BUFFER, instanceTexIndexVBO);
	glBufferData(GL_ARRAY_BUFFER, textureIndices.size() * sizeof(float), textureIndices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, instanceColorVBO);
	glBufferData(GL_ARRAY_BUFFER, instanceColors.size() * sizeof(glm::vec4), instanceColors.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void drawInstancedCubes(glm::mat4 view, glm::mat4 pers) {
	if (modelMatrices.empty()) return;

	glUseProgram(shaderprogram);

	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(pers));

	glUniform1i(texlocation, 0);
	glActiveTexture(GL_TEXTURE0); 
	glBindTexture(GL_TEXTURE_2D_ARRAY, textureArrayID);

	glBindVertexArray(VAO);
	glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, modelMatrices.size());
}




void CubeTransform(int cubeID, glm::vec3 scaling, glm::vec3 position){
	if(cubeID<0 || cubeID>=modelMatrices.size()) return;

	glm::mat4 temp = glm::mat4(1.0f);
	temp = glm::translate(temp, position);
	temp = glm::scale(temp, scaling);
	modelMatrices[cubeID] = temp;
}

void CubeTransform(int cubeID, glm::vec3 scaling){
	if(cubeID<0 || cubeID>=modelMatrices.size()) return;

	glm::mat4 temp = glm::mat4(1.0f);
	temp = glm::scale(temp, scaling);
	modelMatrices[cubeID] = temp;
}
void setCubeColor(int id, glm::vec4 color){
	if(id<0 || id>=modelMatrices.size()) return;
	instanceColors[id] = color;
}

glm::vec3 getcubepos(int id){
	return glm::vec3(modelMatrices[id][3]);
}
glm::vec3 getcubesize(int id){
	glm::vec3 scale;
	scale.x = glm::length(glm::vec3(modelMatrices[id][0]));
	scale.y = glm::length(glm::vec3(modelMatrices[id][1]));
	scale.z = glm::length(glm::vec3(modelMatrices[id][2]));
	return scale;
}





void updatecubebyid(int id){
	if(id<0 || id>=modelMatrices.size()) return;
	glBindBuffer(GL_ARRAY_BUFFER, instanceMatrixVBO);
	glBufferSubData(GL_ARRAY_BUFFER, id*sizeof(glm::mat4), sizeof(glm::mat4), &modelMatrices[id]);

	glBindBuffer(GL_ARRAY_BUFFER, instanceMatrixVBO);
	glBufferSubData(GL_ARRAY_BUFFER, id*sizeof(glm::vec4), sizeof(glm::vec4), &instanceColors[id]);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void updatecubeall(){
	glBindBuffer(GL_ARRAY_BUFFER, instanceMatrixVBO);
	glBufferData(GL_ARRAY_BUFFER, modelMatrices.size()*sizeof(glm::mat4), modelMatrices.data(), GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}
