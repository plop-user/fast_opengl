#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <stdio.h>
#include <iostream>
#include <string>
#include <glslread.h>
#include <glm/gtc/type_ptr.hpp>
int main()
{
	const int screenwidth = 1280;
	const int screenheight = 720;
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);
	SDL_Window* window = SDL_CreateWindow(
        "OpenGL Test",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
         screenwidth, screenheight,
        SDL_WINDOW_OPENGL
    );
	SDL_GLContext ctx = SDL_GL_CreateContext(window);
	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
		printf("Failed to init GLAD\n");
		return -1;
    }
	glEnable(GL_MULTISAMPLE);

	printf("OpenGL version: %s\n", glGetString(GL_VERSION));


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
	unsigned int shaderprogram = glCreateProgram();
	glAttachShader(shaderprogram, testvertex);
	glAttachShader(shaderprogram, fragshader);
	glLinkProgram(shaderprogram);

//	float vertices[] = {
//         0.5f,  0.5f, 0.0f,  // top right
//         0.5f, -0.5f, 0.0f,  // bottom right
//        -0.5f, -0.5f, 0.0f,  // bottom left
//        -0.5f,  0.5f, 0.0f,
//	0.5f,   0.5f, -0.5f,// top left
//	-0.5f, 0.5f, -0.5f,
//	0.5f, -0.5f, -0.5f,
//	0.5f, -0.5f, -0.5f
//	};
//	unsigned int indices[] = {  // note that we start from 0!
//        0, 1, 3,  // first Triangle
//        1, 2, 3,   // second Triangle
//	4, 5, 7,
//	8, 6, 4
//	
//
//	};

//float vertices[] = {
//    // Front face (Z = 0.5)
//    -0.5f, -0.5f,  0.5f, // 0: Bottom-Left
//     0.5f, -0.5f,  0.5f, // 1: Bottom-Right
//     0.5f,  0.5f,  0.5f, // 2: Top-Right
//    -0.5f,  0.5f,  0.5f, // 3: Top-Left
//
//    // Back face (Z = -0.5)
//    -0.5f, -0.5f, -0.5f, // 4: Bottom-Left
//     0.5f, -0.5f, -0.5f, // 5: Bottom-Right
//     0.5f,  0.5f, -0.5f, // 6: Top-Right
//    -0.5f,  0.5f, -0.5f  // 7: Top-Left
//};
//
//unsigned int indices[] = {
//    // Front Face (Positive Z)
//    0, 1, 2,
//    2, 3, 0,
//
//    // Right Face (Positive X)
//    1, 5, 6,
//    6, 2, 1,
//
//    // Back Face (Negative Z)
//    // Note: The order seems reversed to maintain Counter-Clockwise winding
//    // when looking at the back of the object.
//    7, 6, 5,
//    5, 4, 7,
//
//    // Left Face (Negative X)
//    4, 0, 3,
//    3, 7, 4,
//
//    // Top Face (Positive Y)
//    3, 2, 6,
//    6, 7, 3,
//
//    // Bottom Face (Negative Y)
//    4, 5, 1,
//    1, 0, 4
//};

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

		// Create 100 offsets
glm::vec2 translations[100];
int index = 0;
float offset = 0.1f;
for(int y = -10; y < 10; y += 2)
{
    for(int x = -10; x < 10; x += 2)
    {
        glm::vec2 translation;
        translation.x = (float)x / 1.0f + offset;
        translation.y = (float)y / 1.0f + offset;
        translations[index++] = translation;
    }
}
	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(sizeof(float)));
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);


	unsigned int instanceVBO;
	glGenBuffers(1, &instanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * 100, &translations[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(VAO); // Open your existing VAO
	
	// Bind the instance VBO
	glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
	
	// Set up the attribute pointer (Location 2, 2 floats, etc.)
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	
	// THIS IS THE MAGIC LINE
	// 2 = The attribute location
	// 1 = Advance this attribute once per DIVISOR (once per instance)
	glVertexAttribDivisor(1, 1); 
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	// model matrix for sqaure
	float angle = glm::radians(45.0f);	
	glm::mat4 model = glm::mat4(1.0f);              // identity
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0, 0, 1));
	model = glm::scale(model, glm::vec3(1.0f));
	// view matrix (basically camera)
	glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -10.0f));
	float aspectratio = (float)screenwidth/(float)screenheight;
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectratio, 0.1f, 100.0f);
 	 
  
	int modelLoc = glGetUniformLocation(shaderprogram, "model");
	int viewLoc  = glGetUniformLocation(shaderprogram, "view");
	int projLoc  = glGetUniformLocation(shaderprogram, "projection");
	glEnable(GL_DEPTH_TEST);


	SDL_GetPerformanceFrequency();
	SDL_GetPerformanceCounter();
	float LAST = 0;
	float NOW = 0;
	float time = 0;
	while (1) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) return 0;
		if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED){
				int newWidth = e.window.data1;
				int newHeight = e.window.data2;
				glViewport(0,0,newWidth, newHeight);

				if(newHeight>0){float newasp = (float)newWidth/(float)newHeight;
				projection = glm::perspective(glm::radians(45.0f), newasp,0.1f, 100.0f);
				aspectratio = newasp;
				}

			}        }

	LAST= NOW;
	NOW = SDL_GetPerformanceCounter();
	const float deltatime = (float)(NOW-LAST)/(float)SDL_GetPerformanceFrequency();
	time = time + deltatime;
	if(time>0.01f){
	
//	model = glm::translate(model, glm::vec3(-0.01f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(1.0f), glm::vec3(0, 1, 0));
	model = glm::scale(model, glm::vec3(1.0001f));
	time = 0;}

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderprogram);
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc,1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc,1,GL_FALSE, glm::value_ptr(projection));
	glBindVertexArray(VAO);
//	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, 100);
	
	SDL_GL_SwapWindow(window);
    }
	glDeleteVertexArrays(1,&VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteProgram(shaderprogram);
}

