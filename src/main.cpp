#include <SDL2/SDL.h>
#include <vulkan/vulkan.hpp>

#include <iostream>
#include <vector>
#include <cstdint>

int SDL_main(int argc, char* argv[]) {
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

	vk::ApplicationInfo appInfo = vk::ApplicationInfo("Voxel Engine", 0, "LunarG SDK", 1, VK_API_VERSION_1_0);

	vk::InstanceCreateInfo instInfo = vk::InstanceCreateInfo(vk::InstanceCreateFlags(), &appInfo, 0, nullptr, 0, nullptr);

	vk::Instance instance;

	try {
		instance = vk::createInstance(instInfo);
	}
	catch (std::exception e) {
		std::cerr << "vk_CreateInstance Error: " << e.what() << std::endl;
	}

#ifdef _WIN32
	system("pause");
#endif

	SDL_Quit();
	
	instance.destroy();

	return 0;
}