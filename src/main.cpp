#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp> 
#include <stdio.h>
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
	int succ;
	glCompileShader(testvertex);

	// doing frag shader
	std::string fragtext = readFile("shaders/frag.glsl");
	const char* fragtextc = fragtext.c_str();
	unsigned int fragshader = glad_glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragshader, 1, &fragtextc, NULL);
	glCompileShader(fragshader);

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
//
float vertices[] = {
    // Front face (Z = 0.5)
    -0.5f, -0.5f,  0.5f, // 0: Bottom-Left
     0.5f, -0.5f,  0.5f, // 1: Bottom-Right
     0.5f,  0.5f,  0.5f, // 2: Top-Right
    -0.5f,  0.5f,  0.5f, // 3: Top-Left

    // Back face (Z = -0.5)
    -0.5f, -0.5f, -0.5f, // 4: Bottom-Left
     0.5f, -0.5f, -0.5f, // 5: Bottom-Right
     0.5f,  0.5f, -0.5f, // 6: Top-Right
    -0.5f,  0.5f, -0.5f  // 7: Top-Left
};

unsigned int indices[] = {
    // Front Face (Positive Z)
    0, 1, 2,
    2, 3, 0,

    // Right Face (Positive X)
    1, 5, 6,
    6, 2, 1,

    // Back Face (Negative Z)
    // Note: The order seems reversed to maintain Counter-Clockwise winding
    // when looking at the back of the object.
    7, 6, 5,
    5, 4, 7,

    // Left Face (Negative X)
    4, 0, 3,
    3, 7, 4,

    // Top Face (Positive Y)
    3, 2, 6,
    6, 7, 3,

    // Bottom Face (Negative Y)
    4, 5, 1,
    1, 0, 4
};



	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// model matrix for sqaure
	float angle = glm::radians(45.0f);	
	glm::mat4 model = glm::mat4(1.0f);              // identity
	model = glm::translate(model, glm::vec3(0.5f, 0.0f, 0.0f));
	model = glm::rotate(model, 0.0f, glm::vec3(0, 0, 1));
	model = glm::scale(model, glm::vec3(0.3f));
	// view matrix (basically camera)
	glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f));
	float aspectratio = (float)screenwidth/(float)screenheight;
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectratio, 0.1f, 100.0f);
  
  
	int modelLoc = glGetUniformLocation(shaderprogram, "model");
	int viewLoc  = glGetUniformLocation(shaderprogram, "view");
	int projLoc  = glGetUniformLocation(shaderprogram, "projection");
	glEnable(GL_DEPTH_TEST);

	// tring to render multiple sqaures with instanced rendering
	glm::mat4 test[10];
	for(int i = 0; i<10; i++){
		test[i] = model;
		test[i] = glm::translate(test[i], glm::vec3(0.5f+(float)i));
	}



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
	
//	model = glm::translate(model, glm::vec3(0.0f, 0.00f, -0.1f));
	model = glm::rotate(model, glm::radians(1.0f), glm::vec3(0, 0, 1));
//	model = glm::scale(model, glm::vec3(0.99f));
	time = 0;}

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(shaderprogram);
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc,1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc,1,GL_FALSE, glm::value_ptr(projection));
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	
	
	SDL_GL_SwapWindow(window);
    }
	glDeleteVertexArrays(1,&VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteProgram(shaderprogram);
}

