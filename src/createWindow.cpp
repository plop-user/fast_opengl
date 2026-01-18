#include <print>
#include <SDL2/SDL.h>


SDL_Window* createwindow(const char* title, int width, int height){
	if (SDL_Init(SDL_INIT_VIDEO)){
		std::println("SDL failed to start {}", SDL_GetError());
}
	SDL_Window* window = SDL_CreateWindow(title,
				SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED,
				width,
				height,
				SDL_WINDOW_SHOWN);

	return window;

}


