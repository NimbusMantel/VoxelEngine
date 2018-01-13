#include <SDL2/SDL.h>
#include <vulkan/vulkan.hpp>

#include <iostream>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <functional>

const uint16_t WIDTH = 640;
const uint16_t HEIGHT = 360;

SDL_Window* window;

vk::Instance instance;
std::vector<vk::ExtensionProperties> extensions;
vk::DebugReportCallbackEXT callback;
vk::Device device;

#ifndef NDEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT, uint64_t, size_t, int32_t, const char*, const char* msg, void*) {
	throw std::runtime_error(std::string("VK_ValidationLayer Error: ") + msg);
}
#endif

void initSDL() {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		throw std::runtime_error("SDL_Init Error: " + (std::string)SDL_GetError());
	}

	window = SDL_CreateWindow("Voxel Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_VULKAN);

	if (window == nullptr) {
		throw std::runtime_error(std::string("SDL_CreateWindow Error:") + SDL_GetError());
	}
}

void initVulkan() {
	vk::ApplicationInfo appInfo = vk::ApplicationInfo("Voxel Engine", VK_MAKE_VERSION(1, 0, 0), "LunarG SDK", VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_0);

#ifdef NDEBUG
	std::vector<const char*> layers = {};
#else
	std::vector<const char*> layers = {
		"VK_LAYER_LUNARG_standard_validation"
	};
#endif

	std::vector<const char*> extSDL;

	uint32_t extCount = 0;

	if (!SDL_Vulkan_GetInstanceExtensions(window, &extCount, nullptr)) {
		throw std::runtime_error(std::string("SDL_Vulkan_GetInstanceExtensions Error: ") + SDL_GetError());
	}

	extSDL = std::vector<const char*>(extCount);

	SDL_Vulkan_GetInstanceExtensions(window, &extCount, extSDL.data());

#ifndef NDEBUG
	extSDL.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

	vk::InstanceCreateInfo instInfo = vk::InstanceCreateInfo(vk::InstanceCreateFlags(), &appInfo, static_cast<uint32_t>(layers.size()), layers.data(), static_cast<uint32_t>(extSDL.size()), extSDL.data());

	try {
		instance = vk::createInstance(instInfo);
	}
	catch (std::exception e) {
		throw std::runtime_error(std::string("VK_CreateInstance Error: ") + e.what());
	}

#ifndef NDEBUG
	vk::DebugReportCallbackCreateInfoEXT callInfo = vk::DebugReportCallbackCreateInfoEXT(vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning, debugCallback);

	vk::Result res = vk::Result(((PFN_vkCreateDebugReportCallbackEXT)instance.getProcAddr("vkCreateDebugReportCallbackEXT"))(instance, reinterpret_cast<const VkDebugReportCallbackCreateInfoEXT*>(&callInfo), nullptr, reinterpret_cast<VkDebugReportCallbackEXT*>(&callback)));

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDebugReportCallbackEXT Error: ") + vk::to_string(res));
	}
#endif

	uint32_t physCount = 0;

	instance.enumeratePhysicalDevices(&physCount, nullptr);

	if (physCount == 0) {
		throw std::runtime_error(std::string("VK_EnumeratePhysicalDevices Error: ") + "ErrorVulkanGPUNotPresent");
	}

	std::vector<vk::PhysicalDevice> physDevs(physCount);

	instance.enumeratePhysicalDevices(&physCount, physDevs.data());

	for (const vk::PhysicalDevice dev : physDevs) {
		// TO DO: check to see if dev is a suitable device

		goto deviceFound;
	}

	throw std::runtime_error(std::string("VK_EnumeratePhysicalDevices Error: ") + "ErrorSuitableGPUNotPresent");

deviceFound:

	// TO DO: start using device

	std::cout << "";
}

void update() {
	// TO DO
}

void cleanUp() {
#ifndef NDEBUG
	((PFN_vkDestroyDebugReportCallbackEXT)instance.getProcAddr("vkDestroyDebugReportCallbackEXT"))(instance, static_cast<VkDebugReportCallbackEXT>(callback), nullptr);
#endif

	instance.destroy();

	SDL_DestroyWindow(window);
	SDL_Quit();

#ifdef _WIN32
	system("pause");
#endif
}

int SDL_main(int argc, char* argv[]) {
	try {
		initSDL();

		initVulkan();

		update();
	}
	catch (std::runtime_error e) {
		std::cerr << e.what() << std::endl;
	}
	
	cleanUp();

	return 0;
}