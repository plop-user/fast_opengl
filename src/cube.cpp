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


#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>


static unsigned int VAO;
static unsigned int shaderprogram;
static int modelLoc;
static int viewLoc;
static glm::mat4 model;
static int projLoc; 
static int texlocation;
static GLuint textureID;






void createcube(){
	std::string myvertext = readFile("shaders/vet.glsl");
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
	std::string fragtext = readFile("shaders/frag.glsl");
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

	SDL_Surface* cubef = IMG_Load("assets/map/texture_check.png");
	if(!cubef){std::cout << "Image loading error";}

	SDL_Surface* cubefs = SDL_ConvertSurfaceFormat(cubef, SDL_PIXELFORMAT_RGBA32, 0);
	SDL_FreeSurface(cubef);

	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cubefs->w, cubefs->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, cubefs->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	SDL_FreeSurface(cubefs);

glm::vec2 translations[1000000];
int index = 0;
float offset = 0.1f;
for(int y = -1000; y < 1000; y += 2)
{
    for(int x = -1000; x < 1000; x += 2)
    {
        glm::vec2 translation;
        translation.x = (float)x / 1.2f + offset;
        translation.y = (float)y / 1.2f + offset;
        translations[index++] = translation;
    }
}
	unsigned int VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindVertexArray(VAO);
glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),(void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);



	texlocation = glGetUniformLocation(shaderprogram, "cubetex");
	model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));

	unsigned int instanceVBO;
	glGenBuffers(1, &instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * 1000000, &translations[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(VAO); // Open your existing VAO
	
	// Bind the instance VBO
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	
	// THIS IS THE MAGIC LINE
	// 2 = The attribute location
	// 1 = Advance this attribute once per DIVISOR (once per instance)
	glVertexAttribDivisor(1, 1); 
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);



	float angle = glm::radians(45.0f);	
	model = glm::mat4(1.0f);   	



	modelLoc = glGetUniformLocation(shaderprogram, "model");
	viewLoc  = glGetUniformLocation(shaderprogram, "view");
	projLoc  = glGetUniformLocation(shaderprogram, "projection");


}


void cubedraw(glm::mat4 view, glm::mat4 pers, float time){


	glUseProgram(shaderprogram);
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc,1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc,1,GL_FALSE, glm::value_ptr(pers));

	glUniform1i(texlocation, 0);
	glActiveTexture(GL_TEXTURE0); 
	glBindTexture(GL_TEXTURE_2D, textureID);
	if(time>0.001f){
		model = glm::rotate(model,0.01f, glm::vec3(1.0f,1.0f, 1.0f));
	}
  
	glBindVertexArray(VAO);
//	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, 1000000);
	

}
