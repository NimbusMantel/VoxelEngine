#include <SDL2/SDL.h>

#include <iostream>

int main(int argc, char* argv[]) {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;

		return -1;
	}

	SDL_Window* window = SDL_CreateWindow("Voxel Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_VULKAN);

	if (window == nullptr) {
		std::cerr << "SDL_CreateWindow Error:" << SDL_GetError() << std::endl;

		SDL_Quit();

		return -1;
	}

#ifdef _WIN32
	system("pause");
#endif

	SDL_Quit();

	return 0;
}