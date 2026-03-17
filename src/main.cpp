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
#include <shapes.h>
#include <obj.h>

int main()
{
	const int screenwidth = 1280;
	const int screenheight = 720;
	SDL_Init(SDL_INIT_VIDEO);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
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







	glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -20.0f));
	float aspectratio = (float)screenwidth/(float)screenheight;
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectratio, 0.01f, 1000.0f);

	glm::vec3 camerapos(0.0f, 0.0f, -20.0f);
	glm::vec3 dir(0.0f, 1.0f, 0.0f);
	glm::vec3 targetdir(0.0f, 0.0f, 0.0f);


	float yaw   = -90.0f; // -90 points to negative Z (default forward)
	float pitch =  0.0f;


	view = glm::lookAt(camerapos, targetdir, dir);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	creategrid();
	//	createcube();
	//createsphere();
	Uint64 NOW = SDL_GetPerformanceCounter();
	Uint64 LAST = 0;
	float deltatime = 0;
	SDL_SetRelativeMouseMode(SDL_TRUE);



	// cube creating function
	std::vector<std::string> textures = {
		"assets/map/texture_check.png",  // Index 0
		//  "assets/map/stone.png", // Index 1
		//   "assets/map/grass.png"  // Index 2
	};
	std::vector<std::string> tt = {
	};





	initcubesystem(textures);
	initspheresystem(tt, 3);
	int c1 = addCube(1.0f, glm::vec3(0.0f, 0.0f, 0.0f), 0.0f);
	addCube(1.0f, glm::vec3(1.0f,1.0f,1.0f), 0.0f);
	addCube(1.0f, glm::vec3(2,0,0), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	int s1 = addSphere(2.0, glm::vec3(0.0f,0.0f,0.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	addSphere(3.0, glm::vec3(2.0f,2.0f,2.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
	bufferInstanceData();
	bufferInstanceDataSphere();





	initSystem({"assets/check.obj"}, {"assets/map/nightearth.png"});
	addObject(0, {0,0,0}, 1.0, 0);
	
	uploadInstanceData();
	buildMDI();





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

			} 
			if (e.type == SDL_MOUSEMOTION) {
				float xoffset = e.motion.xrel;
				float yoffset = -e.motion.yrel; 
				float sensitivity = 0.1f;

				yaw   += xoffset * sensitivity;
				pitch += yoffset * sensitivity;

				if(pitch > 89.0f)  pitch = 89.0f;
				if(pitch < -89.0f) pitch = -89.0f;

				glm::vec3 direction;
				direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
				direction.y = sin(glm::radians(pitch));
				direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
				targetdir = glm::normalize(direction);
			}
		}

		LAST = NOW;
		NOW = SDL_GetPerformanceCounter();

		// Calculate time passed in seconds
		deltatime = (float)((NOW - LAST) * 1) / (float)SDL_GetPerformanceFrequency();

		const Uint8* currentkeystates = SDL_GetKeyboardState(NULL);



		glm::vec3 worldUp(0.0f, 1.0f, 0.0f); 
		glm::vec3 right = glm::normalize(glm::cross(targetdir, worldUp));
		glm::vec3 flatForward = glm::normalize(glm::vec3(targetdir.x, 0.0f, targetdir.z));
		float cameraSpeed = deltatime*8; 
		if(currentkeystates[SDL_SCANCODE_W]){
			// Move in the direction we are looking

			camerapos += flatForward * cameraSpeed;
		}

		if(currentkeystates[SDL_SCANCODE_S]){
			camerapos -= flatForward * cameraSpeed;
		}
		if(currentkeystates[SDL_SCANCODE_D]){
			// Move along the calculated Right vector
			camerapos += right * cameraSpeed;
		}
		if(currentkeystates[SDL_SCANCODE_A]){
			camerapos -= right * cameraSpeed;
		}

		// Up and Down (Fly up/down globally)
		if(currentkeystates[SDL_SCANCODE_SPACE]){
			camerapos += worldUp * cameraSpeed;
		}
		if(currentkeystates[SDL_SCANCODE_LSHIFT]){
			camerapos -= worldUp * cameraSpeed;
		}
		view = glm::lookAt(camerapos, camerapos+targetdir, worldUp);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		drawScene(view, projection);
		griddraw(view, projection);
		//	cubedraw(view, projection, deltatime);
		//spheredraw(view, projection, time);
//		drawInstancedCubes(view, projection);
//		drawInstancedSpheres(view, projection);
		//	view = glm::translate(view, glm::vec3(0.0f, -0.001f, 0.0f));
		//	model = glm::rotate(model, glm::radians(1.0f), glm::vec3(1, 0, 0));
		//	model = glm::scale(model, glm::vec3(1.001f));


		SDL_GL_SwapWindow(window);
	}
	//	glDeleteVertexArrays(1,&VAO);
	//	glDeleteBuffers(1, &VBO);
	//	glDeleteBuffers(1, &EBO);
	//	glDeleteProgram(shaderprogram);
}

