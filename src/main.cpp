#include <SDL2/SDL.h>
#include <vulkan/vulkan.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

#include <iostream>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <functional>
#include <set>
#include <algorithm>
#include <fstream>

#include "camera.hpp"

#define VOXEL_DEPTH 16

uint32_t tmp = (127 - VOXEL_DEPTH) << 23;

const uint16_t WIDTH = 640;
const uint16_t HEIGHT = 360;
const uint8_t  FOV = 70;
const float SCALE = *((float*)(&tmp));

#ifdef NDEBUG
const std::vector<const char*> layers = {};
#else
const std::vector<const char*> layers = {
	"VK_LAYER_LUNARG_standard_validation"
};
#endif

const std::vector<const char*> devExt = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

SDL_Window* window;

bool isFullscreen = false;

struct Mouse {
	glm::uvec2 pos;

	bool isDragging;
} mouse;

vk::Instance instance;
vk::DebugReportCallbackEXT callback;

vk::SurfaceKHR surface;

vk::PhysicalDevice physical;

struct QueueFamilies {
	uint32_t computeFamily;
	uint32_t transferFamily;
	uint32_t presentFamily;

	std::set<uint32_t> uniqueFamilies;
} queueIndices;

vk::Device device;

vk::PhysicalDeviceMemoryProperties memoryProps;

vk::Queue computeQueue;
vk::Queue transferQueue;
vk::Queue presentationQueue;

vk::SwapchainKHR swapChain;

std::vector<vk::Image> swapChainImages;

vk::Format swapChainFormat;
vk::Extent2D swapChainExtent;

vk::Viewport viewport;

vk::Semaphore imageAvailable;
vk::Semaphore renderFinished;

struct OffscreenPass {
	vk::CommandPool pool;
	vk::CommandBuffer buffer;

	vk::Semaphore semaphore;

	struct HDRImage {
		vk::DeviceSize size = sizeof(glm::i16vec4) * WIDTH * HEIGHT;

		vk::Buffer image;
		vk::DeviceMemory memory;
	} hdrImage;

	struct LDRImage {
		vk::Format format = vk::Format::eR8G8B8A8Unorm;
		vk::Extent3D extent = vk::Extent3D(WIDTH, HEIGHT, 1);

		vk::Image image;
		vk::DeviceMemory memory;
		vk::ImageView view;
	} ldrImage;

	struct VoxelUpdater {
		vk::PipelineLayout layout;
		vk::Pipeline pipeline;

		struct Descriptor {
			vk::DescriptorPool pool;
			vk::DescriptorSetLayout layout;
			vk::DescriptorSet set;
		} descriptor;
	} voxelUpdater;

	struct RayTracer {
		vk::PipelineLayout layout;
		vk::Pipeline pipeline;

		struct Descriptor {
			vk::DescriptorPool pool;
			vk::DescriptorSetLayout layout;
			vk::DescriptorSet set;
		} descriptor;

		struct Queue {
			vk::DeviceSize size = sizeof(uint32_t);

			vk::Buffer buffer;
			vk::DeviceMemory memory;
		} queue;

		struct Visibility {
			vk::DeviceSize size = sizeof(uint8_t) << 21;

			vk::Buffer buffer;
			vk::DeviceMemory memory;
		} visibility;

		struct Constants {
			glm::mat3  rot = glm::mat3(1.0f);
			glm::vec3  pos = glm::vec3(0.0f, 1.0f, 0.0f);
			glm::uvec2 siz = glm::uvec2(WIDTH, HEIGHT);
			float      fov = tanf(float(M_PI) * FOV / 360.0f);
		} constants;
	} rayTracer;

	struct ToneMapper {
		vk::PipelineLayout layout;
		vk::Pipeline pipeline;

		struct Descriptor {
			vk::DescriptorPool pool;
			vk::DescriptorSetLayout layout;
			vk::DescriptorSet set;
		} descriptor;
	} toneMapper;
} offscreenPass;

struct TransferPass {
	vk::CommandPool pool;
	std::vector<vk::CommandBuffer> buffers;
} transferPass;

struct VoxelBuffer {
	vk::Extent3D extent = vk::Extent3D(256, 256, 256);

	vk::DeviceMemory memory;

	vk::Format strFormat = vk::Format::eR32Uint;
	vk::Image strImage;
	vk::ImageView strView;
} voxelBuffer;

#ifndef NDEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT, uint64_t, size_t, int32_t, const char*, const char* msg, void*) {
	throw std::runtime_error(std::string("VK_ValidationLayer Error: ") + msg);
}
#endif

static std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("I/O readFile Error: Failed to open file " + filename);
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

int SDL_main(int argc, char* argv[]);

static void initSDL();
static void initVulkan();
static void update();
static void cleanup();

static void initState();
static void updateState();
static void changeFullscreen(bool fullscreen);
static void drawFrame();

static void createInstance();
static void createDebugCallback();
static void createSurface();
static void pickPhysicalDevice();
static void createLogicalDevice();

static void createSwapChain();

static void createImages();
static void createImageViews();
static void createBuffers();

static void createDescriptorPools();
static void createDescriptorSets();
static void createOffscreenPipeline();

static void createCommandPools();
static void createRenderCommandBuffer();
static void recordRenderCommandBuffer();
static void createTransferCommandBuffers();

static void createSemaphores();

static void recreateSwapChain();
static void cleanupSwapChain();

int SDL_main(int argc, char* argv[]) {
	initState();

	try {
		initSDL();

		initVulkan();

		update();
	}
	catch (std::runtime_error e) {
		std::cerr << e.what() << std::endl;
	}

	try {
		cleanup();
	}
	catch (std::runtime_error e) {
		std::cerr << e.what() << std::endl;
	}

#ifdef _WIN32
	system("pause");
#endif

	return 0;
}

static void initSDL() {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		throw std::runtime_error("SDL_Init Error: " + (std::string)SDL_GetError());
	}

	window = SDL_CreateWindow("Voxel Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_VULKAN);

	if (window == nullptr) {
		throw std::runtime_error(std::string("SDL_CreateWindow Error:") + SDL_GetError());
	}
}

static void initVulkan() {
	createInstance();

	createDebugCallback();

	createSurface();

	pickPhysicalDevice();
	createLogicalDevice();

	createSwapChain();
	createImages();
	createImageViews();

	createBuffers();

	createDescriptorPools();
	createDescriptorSets();
	createOffscreenPipeline();

	createCommandPools();
	createRenderCommandBuffer();
	recordRenderCommandBuffer();
	createTransferCommandBuffers();

	createSemaphores();
}

static void update() {
	uint32_t width, height, size;

	SDL_Event event;

	bool quit = false;
	
	while (!quit) {
		SDL_GetWindowSize(window, (int*)&width, (int*)&height);
		size = height ^ ((width ^ height) & -(width < height));

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				quit = true;
			}
			else if (event.type == SDL_WINDOWEVENT) {
				vk::SurfaceCapabilitiesKHR surfaceCapabilities = physical.getSurfaceCapabilitiesKHR(surface);

				if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() && surfaceCapabilities.currentExtent.height != std::numeric_limits<uint32_t>::max()) {
					swapChainExtent = surfaceCapabilities.currentExtent;
				}
				else {
					swapChainExtent = vk::Extent2D(std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, width)),
						std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, height)));
				}
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_ESCAPE) {
					if (isFullscreen) {
						changeFullscreen(false);
					}
					else {
						quit = true;
					}
				}
				else if (event.key.keysym.sym == SDLK_f) {
					changeFullscreen(!isFullscreen);
				}
			}
			else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
				mouse.isDragging = true;

				mouse.pos = glm::uvec2(event.button.x, height - 1 - event.button.y);

				SDL_SetRelativeMouseMode(SDL_TRUE);
			}
			else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
				mouse.isDragging = false;

				mouse.pos = glm::uvec2(event.button.x, height - 1 - event.button.y);

				SDL_SetRelativeMouseMode(SDL_FALSE);
			}
			else if (event.type == SDL_MOUSEMOTION) {
				mouse.pos = glm::uvec2(event.motion.x, height - 1 - event.motion.y);

				if (mouse.isDragging) {
					camera::rot(glm::vec2(event.motion.xrel, -event.motion.yrel) * -0.02f * (float)M_PI / (0.5f * size));
				}
			}
			else if (event.type == SDL_MOUSEWHEEL) {
				camera::rad(event.wheel.y * -0.5f);
			}
		}

		if (swapChainExtent.width > 0 && swapChainExtent.height > 0) {
			updateState();

			drawFrame();
		}
	}

	device.waitIdle();
}

static void cleanup() {
	device.destroySemaphore(imageAvailable, nullptr);
	device.destroySemaphore(renderFinished, nullptr);
	device.destroySemaphore(offscreenPass.semaphore, nullptr);

	device.freeCommandBuffers(offscreenPass.pool, { offscreenPass.buffer });
	device.destroyCommandPool(offscreenPass.pool, nullptr);
	device.freeCommandBuffers(transferPass.pool, transferPass.buffers);
	device.destroyCommandPool(transferPass.pool, nullptr);

	device.destroyPipeline(offscreenPass.voxelUpdater.pipeline, nullptr);
	device.destroyPipelineLayout(offscreenPass.voxelUpdater.layout, nullptr);
	device.destroyPipeline(offscreenPass.rayTracer.pipeline, nullptr);
	device.destroyPipelineLayout(offscreenPass.rayTracer.layout, nullptr);
	device.destroyPipeline(offscreenPass.toneMapper.pipeline, nullptr);
	device.destroyPipelineLayout(offscreenPass.toneMapper.layout, nullptr);

	device.destroyDescriptorSetLayout(offscreenPass.voxelUpdater.descriptor.layout, nullptr);
	device.destroyDescriptorPool(offscreenPass.voxelUpdater.descriptor.pool, nullptr);
	device.destroyDescriptorSetLayout(offscreenPass.rayTracer.descriptor.layout, nullptr);
	device.destroyDescriptorPool(offscreenPass.rayTracer.descriptor.pool, nullptr);
	device.destroyDescriptorSetLayout(offscreenPass.toneMapper.descriptor.layout, nullptr);
	device.destroyDescriptorPool(offscreenPass.toneMapper.descriptor.pool, nullptr);

	device.destroyBuffer(offscreenPass.rayTracer.queue.buffer, nullptr);
	device.freeMemory(offscreenPass.rayTracer.queue.memory, nullptr);
	device.destroyBuffer(offscreenPass.rayTracer.visibility.buffer, nullptr);
	device.freeMemory(offscreenPass.rayTracer.visibility.memory, nullptr);
	device.destroyBuffer(offscreenPass.hdrImage.image, nullptr);
	device.freeMemory(offscreenPass.hdrImage.memory, nullptr);

	device.destroyImageView(offscreenPass.ldrImage.view, nullptr);
	device.destroyImage(offscreenPass.ldrImage.image, nullptr);
	device.freeMemory(offscreenPass.ldrImage.memory, nullptr);

	device.destroyImageView(voxelBuffer.strView, nullptr);
	device.destroyImage(voxelBuffer.strImage, nullptr);
	device.freeMemory(voxelBuffer.memory, nullptr);

	device.destroySwapchainKHR(swapChain, nullptr);

	device.destroy();

#ifndef NDEBUG
	((PFN_vkDestroyDebugReportCallbackEXT)instance.getProcAddr("vkDestroyDebugReportCallbackEXT"))(instance, static_cast<VkDebugReportCallbackEXT>(callback), nullptr);
#endif

	instance.destroySurfaceKHR(surface);
	instance.destroy();

	SDL_DestroyWindow(window);
	SDL_Quit();
}

static void initState() {
	camera::mov(glm::vec3(0.5f, 0.5f, -0.5f));
	camera::rad(10.0f);

	camera::upd(offscreenPass.rayTracer.constants.pos, offscreenPass.rayTracer.constants.rot);

	offscreenPass.rayTracer.constants.pos = offscreenPass.rayTracer.constants.pos * SCALE + 1.5f;
}

static void updateState() {
	if (camera::upd(offscreenPass.rayTracer.constants.pos, offscreenPass.rayTracer.constants.rot)) {
		offscreenPass.rayTracer.constants.pos = offscreenPass.rayTracer.constants.pos * SCALE + 1.5f;

		recordRenderCommandBuffer();
	}
}

static void changeFullscreen(bool fullscreen) {
	if (fullscreen == isFullscreen) {
		return;
	}

	isFullscreen = fullscreen;

	SDL_SetWindowFullscreen(window, isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);

	recreateSwapChain();
}

static void drawFrame() {
	presentationQueue.waitIdle();

	uint32_t imageIndex;

	vk::Result res = device.acquireNextImageKHR(swapChain, std::numeric_limits<uint64_t>::max(), imageAvailable, nullptr, &imageIndex);

	if (res == vk::Result::eErrorOutOfDateKHR) {
		recreateSwapChain();

		return;
	}
	else if (res != vk::Result::eSuccess && res != vk::Result::eSuboptimalKHR) {
		throw std::runtime_error(std::string("VK_AcquireNextImageKHR Error: ") + vk::to_string(res));
	}

	vk::PipelineStageFlags waitStages[] = {
		vk::PipelineStageFlagBits::eTopOfPipe
	};

	vk::SubmitInfo submitInfo(1, &imageAvailable, waitStages, 1, &offscreenPass.buffer, 1, &offscreenPass.semaphore);

	res = computeQueue.submit(1, &submitInfo, nullptr);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_QueueSubmit Error: ") + vk::to_string(res));
	}

	waitStages[0] = vk::PipelineStageFlagBits::eComputeShader;

	submitInfo = vk::SubmitInfo(1, &offscreenPass.semaphore, waitStages, 1, &transferPass.buffers[imageIndex], 1, &renderFinished);

	res = transferQueue.submit(1, &submitInfo, nullptr);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_QueueSubmit Error: ") + vk::to_string(res));
	}

	vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR(1, &renderFinished, 1, &swapChain, &imageIndex, nullptr);

	res = presentationQueue.presentKHR(&presentInfo);

	if (res == vk::Result::eErrorOutOfDateKHR || res == vk::Result::eSuboptimalKHR) {
		recreateSwapChain();
	}
	else if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_QueuePresentKHR Error: ") + vk::to_string(res));
	}

	presentationQueue.waitIdle();
}

static void createInstance() {
	vk::ApplicationInfo appInfo = vk::ApplicationInfo("Voxel Engine", VK_MAKE_VERSION(1, 0, 0), "LunarG SDK", VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_0);

	std::vector<const char*> instExt;

	uint32_t extCount = 0;

	if (!SDL_Vulkan_GetInstanceExtensions(window, &extCount, nullptr)) {
		throw std::runtime_error(std::string("SDL_Vulkan_GetInstanceExtensions Error: ") + SDL_GetError());
	}

	instExt.resize(extCount);

	SDL_Vulkan_GetInstanceExtensions(window, &extCount, instExt.data());

#ifndef NDEBUG
	instExt.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

	vk::InstanceCreateInfo instanceInfo(vk::InstanceCreateFlags(), &appInfo, static_cast<uint32_t>(layers.size()), layers.data(), static_cast<uint32_t>(instExt.size()), instExt.data());

	vk::Result res = vk::createInstance(&instanceInfo, nullptr, &instance);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateInstance Error: ") + vk::to_string(res));
	}
}

static void createDebugCallback() {
#ifndef NDEBUG
	vk::DebugReportCallbackCreateInfoEXT callInfo(vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning, debugCallback);

	vk::Result res = vk::Result(((PFN_vkCreateDebugReportCallbackEXT)instance.getProcAddr("vkCreateDebugReportCallbackEXT"))(instance,
		reinterpret_cast<const VkDebugReportCallbackCreateInfoEXT*>(&callInfo), nullptr, reinterpret_cast<VkDebugReportCallbackEXT*>(&callback)));

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDebugReportCallbackEXT Error: ") + vk::to_string(res));
	}
#endif
}

static void createSurface() {
	if (SDL_Vulkan_CreateSurface(window, instance, reinterpret_cast<VkSurfaceKHR*>(&surface)) != SDL_TRUE) {
		throw std::runtime_error(std::string("SDL_Vulkan_CreateSurface Error: ") + SDL_GetError());
	}
}

static void pickPhysicalDevice() {
	std::vector<vk::PhysicalDevice> physDevs = instance.enumeratePhysicalDevices();

	if (physDevs.size() == 0) {
		throw std::runtime_error(std::string("VK_EnumeratePhysicalDevices Error: ") + "VulkanGPUNotPresent");
	}

	uint32_t maxGPUScore = 0;
	uint32_t gpuScore;

	vk::PhysicalDeviceProperties devProps;
	vk::PhysicalDeviceFeatures devFeats;

	vk::PhysicalDevice selDev;

	std::vector<vk::QueueFamilyProperties> queueFamilies;

	uint32_t computeQueueIndex, transferQueueIndex, presentationQueueIndex;

	uint32_t comQueIdx, traQueIdx, prsQueIdx;
	uint32_t tmp;

	std::vector<vk::ExtensionProperties> avaExt;
	std::set<std::string> reqExt;

	for (const vk::PhysicalDevice dev : physDevs) {
		gpuScore = 0;

		dev.getProperties(&devProps);
		dev.getFeatures(&devFeats);

		if (devProps.deviceType == vk::PhysicalDeviceType::eCpu) {
			continue;
		}

		if (devProps.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
			gpuScore += 1000;
		}

		gpuScore += devProps.limits.maxImageDimension2D;

		queueFamilies = dev.getQueueFamilyProperties();

		comQueIdx = -1;

		tmp = 0;

		for (const vk::QueueFamilyProperties queFamily : queueFamilies) {
			if (queFamily.queueCount > 0) {
				if ((queFamily.queueFlags & vk::QueueFlagBits::eCompute) == vk::QueueFlagBits::eCompute) {
					comQueIdx = tmp;
				}

				if (((queFamily.queueFlags & vk::QueueFlagBits::eTransfer) == vk::QueueFlagBits::eTransfer) && ((queFamily.queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits::eGraphics)) {
					traQueIdx = tmp;

					gpuScore += (traQueIdx == comQueIdx) << 5;
				}

				if (dev.getSurfaceSupportKHR(tmp, surface)) {
					prsQueIdx = tmp;

					gpuScore += (prsQueIdx == comQueIdx) << 5;
				}
			}

			tmp++;
		}

		if (comQueIdx == -1 || traQueIdx == -1 || prsQueIdx == -1) {
			continue;
		}

		avaExt = dev.enumerateDeviceExtensionProperties(nullptr);

		reqExt = std::set<std::string>(devExt.begin(), devExt.end());

		for (const vk::ExtensionProperties ext : avaExt) {
			reqExt.erase(ext.extensionName);
		}

		if (!reqExt.empty()) {
			continue;
		}

		if (dev.getSurfaceFormatsKHR(surface).empty() || dev.getSurfacePresentModesKHR(surface).empty()) {
			continue;
		}

		if ((dev.getSurfaceCapabilitiesKHR(surface).supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst) != vk::ImageUsageFlagBits::eTransferDst) {
			continue;
		}

		if (gpuScore > maxGPUScore) {
			maxGPUScore = gpuScore;

			selDev = dev;

			computeQueueIndex = comQueIdx;
			transferQueueIndex = traQueIdx;
			presentationQueueIndex = prsQueIdx;
		}
	}

	if (maxGPUScore <= 0) {
		throw std::runtime_error(std::string("VK_EnumeratePhysicalDevices Error: ") + "SuitableGPUNotAvailable");
	}

	physical = selDev;

	physical.getMemoryProperties(&memoryProps);

	queueIndices.computeFamily = computeQueueIndex;
	queueIndices.transferFamily = transferQueueIndex;
	queueIndices.presentFamily = presentationQueueIndex;

	queueIndices.uniqueFamilies = { queueIndices.computeFamily, queueIndices.transferFamily, queueIndices.presentFamily };
}

static void createLogicalDevice() {
	std::vector<vk::DeviceQueueCreateInfo> queueInfos;

	float queuePriority = 1.0f;

	for (const uint32_t queFam : queueIndices.uniqueFamilies) {
		queueInfos.push_back(vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), queFam, 1, &queuePriority));
	}

	vk::PhysicalDeviceFeatures deviceFeatures;
	deviceFeatures.shaderStorageImageExtendedFormats = true;
	deviceFeatures.shaderInt64 = true;

	vk::DeviceCreateInfo deviceInfo(vk::DeviceCreateFlags(), static_cast<uint32_t>(queueInfos.size()), queueInfos.data(),
		static_cast<uint32_t>(layers.size()), layers.data(), static_cast<uint32_t>(devExt.size()), devExt.data(), &deviceFeatures);

	vk::Result res = physical.createDevice(&deviceInfo, nullptr, &device);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDevice Error: ") + vk::to_string(res));
	}

	computeQueue = device.getQueue(queueIndices.computeFamily, 0);
	transferQueue = device.getQueue(queueIndices.transferFamily, 0);
	presentationQueue = device.getQueue(queueIndices.presentFamily, 0);
}

static void createSwapChain() {
	vk::SurfaceCapabilitiesKHR surfaceCapabilities = physical.getSurfaceCapabilitiesKHR(surface);
	std::vector<vk::SurfaceFormatKHR> surfaceFormats = physical.getSurfaceFormatsKHR(surface);
	std::vector<vk::PresentModeKHR> presentModes = physical.getSurfacePresentModesKHR(surface);

	vk::SurfaceFormatKHR surfaceFormat;

	if (surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined) {
		surfaceFormat = { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
	}
	else {
		surfaceFormat = surfaceFormats[0];

		for (const vk::SurfaceFormatKHR format : surfaceFormats) {
			if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
				surfaceFormat = format;

				break;
			}
		}
	}

	swapChainFormat = surfaceFormat.format;

	vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;

	for (const vk::PresentModeKHR mode : presentModes) {
		if (mode == vk::PresentModeKHR::eMailbox) {
			presentMode = mode;

			break;
		}
		else if (mode == vk::PresentModeKHR::eImmediate) {
			presentMode = mode;

			break;
		}
	}

	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() && surfaceCapabilities.currentExtent.height != std::numeric_limits<uint32_t>::max()) {
		swapChainExtent = surfaceCapabilities.currentExtent;
	}
	else {
		uint32_t width, height;

		SDL_GetWindowSize(window, (int*)&width, (int*)&height);

		swapChainExtent = vk::Extent2D(std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, width)),
			std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, height)));
	}

	uint32_t imageCount = surfaceCapabilities.minImageCount + 1;

	if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
		imageCount = surfaceCapabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR swapInfo;
	swapInfo.surface = surface;
	swapInfo.minImageCount = imageCount;
	swapInfo.imageFormat = surfaceFormat.format;
	swapInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapInfo.imageExtent = swapChainExtent;
	swapInfo.imageArrayLayers = 1;
	swapInfo.imageUsage = vk::ImageUsageFlagBits::eTransferDst;

	std::vector<uint32_t> queueIndicesData(queueIndices.uniqueFamilies.size());

	queueIndicesData.assign(queueIndices.uniqueFamilies.begin(), queueIndices.uniqueFamilies.end());

	if (queueIndices.uniqueFamilies.size() > 1) {
		swapInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		swapInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueIndicesData.size());
		swapInfo.pQueueFamilyIndices = queueIndicesData.data();
	}
	else {
		swapInfo.imageSharingMode = vk::SharingMode::eExclusive;
		swapInfo.queueFamilyIndexCount = 0;
		swapInfo.pQueueFamilyIndices = nullptr;
	}

	swapInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
	swapInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	swapInfo.presentMode = presentMode;
	swapInfo.clipped = false;
	swapInfo.oldSwapchain = nullptr;

	vk::Result res = device.createSwapchainKHR(&swapInfo, nullptr, &swapChain);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateSwapchainKHR Error: ") + vk::to_string(res));
	}

	swapChainImages = device.getSwapchainImagesKHR(swapChain);
}

static uint32_t getMemoryTypeIndex(uint32_t typeBits, vk::MemoryPropertyFlags flags) {
	uint32_t index = -1;

	for (uint32_t i = 0; i < memoryProps.memoryTypeCount; i++) {
		if ((typeBits & 0x01) == 0x01) {
			if ((memoryProps.memoryTypes[i].propertyFlags & flags) == flags) {
				index = i;

				break;
			}
		}

		typeBits >>= 1;
	}

	if (index == -1) {
		throw std::runtime_error(std::string("VK_GetPhysicalDeviceMemoryProperties Error: ") + "SuitableMemoryNotAvailable");
	}

	return index;
}

static void createImages() {
	// LDR Image

	vk::ImageCreateInfo imageInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, offscreenPass.ldrImage.format, offscreenPass.ldrImage.extent, 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive, 1, &queueIndices.computeFamily, vk::ImageLayout::eUndefined);

	vk::Result res = device.createImage(&imageInfo, nullptr, &offscreenPass.ldrImage.image);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateImage Error: ") + vk::to_string(res));
	}

	vk::MemoryRequirements memReqs = device.getImageMemoryRequirements(offscreenPass.ldrImage.image);

	vk::MemoryAllocateInfo allocInfo(memReqs.size, getMemoryTypeIndex(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

	res = device.allocateMemory(&allocInfo, nullptr, &offscreenPass.ldrImage.memory);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateMemory Error: ") + vk::to_string(res));
	}

	device.bindImageMemory(offscreenPass.ldrImage.image, offscreenPass.ldrImage.memory, 0);

	// Voxel structure

	imageInfo = vk::ImageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e3D, voxelBuffer.strFormat, voxelBuffer.extent, 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eStorage, vk::SharingMode::eExclusive, 1, &queueIndices.computeFamily, vk::ImageLayout::eUndefined);

	res = device.createImage(&imageInfo, nullptr, &voxelBuffer.strImage);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateImage Error: ") + vk::to_string(res));
	}

	memReqs = device.getImageMemoryRequirements(voxelBuffer.strImage);

	allocInfo = vk::MemoryAllocateInfo(memReqs.size, getMemoryTypeIndex(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

	res = device.allocateMemory(&allocInfo, nullptr, &voxelBuffer.memory);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateMemory Error: ") + vk::to_string(res));
	}

	device.bindImageMemory(voxelBuffer.strImage, voxelBuffer.memory, 0);
}

static void createImageViews() {
	// LDR Image

	vk::ImageViewCreateInfo viewInfo(vk::ImageViewCreateFlags(), offscreenPass.ldrImage.image, vk::ImageViewType::e2D, offscreenPass.ldrImage.format, vk::ComponentMapping(vk::ComponentSwizzle::eIdentity,
		vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

	vk::Result res = device.createImageView(&viewInfo, nullptr, &offscreenPass.ldrImage.view);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateImageView Error: ") + vk::to_string(res));
	}

	// Voxel structure

	viewInfo = vk::ImageViewCreateInfo(vk::ImageViewCreateFlags(), voxelBuffer.strImage, vk::ImageViewType::e3D, voxelBuffer.strFormat, vk::ComponentMapping(vk::ComponentSwizzle::eIdentity,
		vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

	res = device.createImageView(&viewInfo, nullptr, &voxelBuffer.strView);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateImageView Error: ") + vk::to_string(res));
	}
}

static void createBuffers() {
	// Ray Queue

	vk::BufferCreateInfo bufferInfo(vk::BufferCreateFlags(), offscreenPass.rayTracer.queue.size, vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive, 1, &queueIndices.computeFamily);

	vk::Result res = device.createBuffer(&bufferInfo, nullptr, &offscreenPass.rayTracer.queue.buffer);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateBuffer Error: ") + vk::to_string(res));
	}

	vk::MemoryRequirements memReqs = device.getBufferMemoryRequirements(offscreenPass.rayTracer.queue.buffer);

	vk::MemoryAllocateInfo allocInfo(memReqs.size, getMemoryTypeIndex(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

	res = device.allocateMemory(&allocInfo, nullptr, &offscreenPass.rayTracer.queue.memory);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateMemory Error: ") + vk::to_string(res));
	}

	device.bindBufferMemory(offscreenPass.rayTracer.queue.buffer, offscreenPass.rayTracer.queue.memory, 0);

	// Visibility bitmask

	bufferInfo = vk::BufferCreateInfo(vk::BufferCreateFlags(), offscreenPass.rayTracer.visibility.size, vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive, 1, &queueIndices.computeFamily);

	res = device.createBuffer(&bufferInfo, nullptr, &offscreenPass.rayTracer.visibility.buffer);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateBuffer Error: ") + vk::to_string(res));
	}

	memReqs = device.getBufferMemoryRequirements(offscreenPass.rayTracer.visibility.buffer);

	allocInfo = vk::MemoryAllocateInfo(memReqs.size, getMemoryTypeIndex(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent |
		vk::MemoryPropertyFlagBits::eHostCached));

	res = device.allocateMemory(&allocInfo, nullptr, &offscreenPass.rayTracer.visibility.memory);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateMemory Error: ") + vk::to_string(res));
	}

	device.bindBufferMemory(offscreenPass.rayTracer.visibility.buffer, offscreenPass.rayTracer.visibility.memory, 0);

	// HDR Image

	bufferInfo = vk::BufferCreateInfo(vk::BufferCreateFlags(), offscreenPass.hdrImage.size, vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive, 1, &queueIndices.computeFamily);

	res = device.createBuffer(&bufferInfo, nullptr, &offscreenPass.hdrImage.image);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateBuffer Error: ") + vk::to_string(res));
	}

	memReqs = device.getBufferMemoryRequirements(offscreenPass.hdrImage.image);

	allocInfo = vk::MemoryAllocateInfo(memReqs.size, getMemoryTypeIndex(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

	res = device.allocateMemory(&allocInfo, nullptr, &offscreenPass.hdrImage.memory);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateMemory Error: ") + vk::to_string(res));
	}

	device.bindBufferMemory(offscreenPass.hdrImage.image, offscreenPass.hdrImage.memory, 0);
}

static void createDescriptorPools() {
	// Voxel Updater

	std::vector<vk::DescriptorPoolSize> poolSizes = {
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1)
	};

	vk::DescriptorPoolCreateInfo poolInfo(vk::DescriptorPoolCreateFlags(), 1, static_cast<uint32_t>(poolSizes.size()), poolSizes.data());

	vk::Result res = device.createDescriptorPool(&poolInfo, nullptr, &offscreenPass.voxelUpdater.descriptor.pool);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDescriptorPool Error: ") + vk::to_string(res));
	}

	// Ray Tracer

	poolSizes = {
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1),
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 3)
	};

	poolInfo = vk::DescriptorPoolCreateInfo(vk::DescriptorPoolCreateFlags(), 1, static_cast<uint32_t>(poolSizes.size()), poolSizes.data());

	res = device.createDescriptorPool(&poolInfo, nullptr, &offscreenPass.rayTracer.descriptor.pool);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDescriptorPool Error: ") + vk::to_string(res));
	}

	// Tone Mapper

	poolSizes = {
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1),
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1)
	};

	poolInfo = vk::DescriptorPoolCreateInfo(vk::DescriptorPoolCreateFlags(), 1, static_cast<uint32_t>(poolSizes.size()), poolSizes.data());

	res = device.createDescriptorPool(&poolInfo, nullptr, &offscreenPass.toneMapper.descriptor.pool);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDescriptorPool Error: ") + vk::to_string(res));
	}
}

static void createDescriptorSets() {
	// Voxel Updater

	std::vector<vk::DescriptorSetLayoutBinding> bindings = {
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr)
	};

	vk::DescriptorSetLayoutCreateInfo layoutInfo(vk::DescriptorSetLayoutCreateFlags(), static_cast<uint32_t>(bindings.size()), bindings.data());

	vk::Result res = device.createDescriptorSetLayout(&layoutInfo, nullptr, &offscreenPass.voxelUpdater.descriptor.layout);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDescriptorSetLayout Error: ") + vk::to_string(res));
	}

	vk::DescriptorSetAllocateInfo allocInfo(offscreenPass.voxelUpdater.descriptor.pool, 1, &offscreenPass.voxelUpdater.descriptor.layout);

	res = device.allocateDescriptorSets(&allocInfo, &offscreenPass.voxelUpdater.descriptor.set);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateDescriptorSets Error: ") + vk::to_string(res));
	}

	// Ray Tracer

	bindings = {
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
		vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
		vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr)
	};

	layoutInfo = vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags(), static_cast<uint32_t>(bindings.size()), bindings.data());

	res = device.createDescriptorSetLayout(&layoutInfo, nullptr, &offscreenPass.rayTracer.descriptor.layout);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDescriptorSetLayout Error: ") + vk::to_string(res));
	}

	allocInfo = vk::DescriptorSetAllocateInfo(offscreenPass.rayTracer.descriptor.pool, 1, &offscreenPass.rayTracer.descriptor.layout);

	res = device.allocateDescriptorSets(&allocInfo, &offscreenPass.rayTracer.descriptor.set);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateDescriptorSets Error: ") + vk::to_string(res));
	}

	// Tone Mapper

	bindings = {
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr)
	};

	layoutInfo = vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags(), static_cast<uint32_t>(bindings.size()), bindings.data());

	res = device.createDescriptorSetLayout(&layoutInfo, nullptr, &offscreenPass.toneMapper.descriptor.layout);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDescriptorSetLayout Error: ") + vk::to_string(res));
	}

	allocInfo = vk::DescriptorSetAllocateInfo(offscreenPass.toneMapper.descriptor.pool, 1, &offscreenPass.toneMapper.descriptor.layout);

	res = device.allocateDescriptorSets(&allocInfo, &offscreenPass.toneMapper.descriptor.set);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateDescriptorSets Error: ") + vk::to_string(res));
	}

	// Write all bindings

	vk::DescriptorImageInfo strInfo(nullptr, voxelBuffer.strView, vk::ImageLayout::eGeneral);
	vk::DescriptorBufferInfo rayInfo(offscreenPass.rayTracer.queue.buffer, 0, offscreenPass.rayTracer.queue.size);
	vk::DescriptorBufferInfo visInfo(offscreenPass.rayTracer.visibility.buffer, 0, offscreenPass.rayTracer.visibility.size);
	vk::DescriptorBufferInfo hdrInfo(offscreenPass.hdrImage.image, 0, offscreenPass.hdrImage.size);
	vk::DescriptorImageInfo ldrInfo(nullptr, offscreenPass.ldrImage.view, vk::ImageLayout::eGeneral);

	std::vector<vk::WriteDescriptorSet> descriptorWrites = {
		vk::WriteDescriptorSet(offscreenPass.voxelUpdater.descriptor.set, 0, 0, 1, vk::DescriptorType::eStorageImage, &strInfo, nullptr, nullptr),
		vk::WriteDescriptorSet(offscreenPass.rayTracer.descriptor.set, 0, 0, 1, vk::DescriptorType::eStorageImage, &strInfo, nullptr, nullptr),
		vk::WriteDescriptorSet(offscreenPass.rayTracer.descriptor.set, 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &rayInfo, nullptr),
		vk::WriteDescriptorSet(offscreenPass.rayTracer.descriptor.set, 2, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &visInfo, nullptr),
		vk::WriteDescriptorSet(offscreenPass.rayTracer.descriptor.set, 3, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &hdrInfo, nullptr),
		vk::WriteDescriptorSet(offscreenPass.toneMapper.descriptor.set, 0, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &hdrInfo, nullptr),
		vk::WriteDescriptorSet(offscreenPass.toneMapper.descriptor.set, 1, 0, 1, vk::DescriptorType::eStorageImage, &ldrInfo, nullptr, nullptr)
	};

	device.updateDescriptorSets(descriptorWrites, nullptr);
}

static vk::ShaderModule createShaderModule(const std::vector<char>& code) {
	vk::ShaderModuleCreateInfo moduleInfo(vk::ShaderModuleCreateFlags(), code.size(), reinterpret_cast<const uint32_t*>(code.data()));

	vk::ShaderModule shaderModule;

	vk::Result res = device.createShaderModule(&moduleInfo, nullptr, &shaderModule);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateShaderModule Error: ") + vk::to_string(res));
	}

	return shaderModule;
}

static void createOffscreenPipeline() {
	// Voxel Updater

	vk::PipelineLayoutCreateInfo layoutInfo(vk::PipelineLayoutCreateFlags(), 1, &offscreenPass.voxelUpdater.descriptor.layout, 0, nullptr);

	vk::Result res = device.createPipelineLayout(&layoutInfo, nullptr, &offscreenPass.voxelUpdater.layout);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreatePipelineLayout Error: ") + vk::to_string(res));
	}

	std::vector<char> compShaderCode = readFile("obj/shaders/voxelUpdater.comp.spv");
	vk::ShaderModule compShaderModule = createShaderModule(compShaderCode);
	vk::PipelineShaderStageCreateInfo compStageInfo(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eCompute, compShaderModule, "main", nullptr);

	vk::ComputePipelineCreateInfo pipelineInfo(vk::PipelineCreateFlags(), compStageInfo, offscreenPass.voxelUpdater.layout, nullptr, -1);

	res = device.createComputePipelines(nullptr, 1, &pipelineInfo, nullptr, &offscreenPass.voxelUpdater.pipeline);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateComputePipelines Error: ") + vk::to_string(res));
	}

	device.destroyShaderModule(compShaderModule, nullptr);

	// Ray Tracer

	vk::PushConstantRange constantRange(vk::ShaderStageFlagBits::eCompute, 0, sizeof(offscreenPass.rayTracer.constants));
	
	layoutInfo = vk::PipelineLayoutCreateInfo(vk::PipelineLayoutCreateFlags(), 1, &offscreenPass.rayTracer.descriptor.layout, 1, &constantRange);

	res = device.createPipelineLayout(&layoutInfo, nullptr, &offscreenPass.rayTracer.layout);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreatePipelineLayout Error: ") + vk::to_string(res));
	}

	compShaderCode = readFile("obj/shaders/rayTracer.comp.spv");
	compShaderModule = createShaderModule(compShaderCode);
	compStageInfo = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eCompute, compShaderModule, "main", nullptr);

	pipelineInfo = vk::ComputePipelineCreateInfo(vk::PipelineCreateFlags(), compStageInfo, offscreenPass.rayTracer.layout, nullptr, -1);

	res = device.createComputePipelines(nullptr, 1, &pipelineInfo, nullptr, &offscreenPass.rayTracer.pipeline);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateComputePipelines Error: ") + vk::to_string(res));
	}

	device.destroyShaderModule(compShaderModule, nullptr);

	// Toner Mapper

	layoutInfo = vk::PipelineLayoutCreateInfo(vk::PipelineLayoutCreateFlags(), 1, &offscreenPass.toneMapper.descriptor.layout, 0, nullptr);

	res = device.createPipelineLayout(&layoutInfo, nullptr, &offscreenPass.toneMapper.layout);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreatePipelineLayout Error: ") + vk::to_string(res));
	}

	compShaderCode = readFile("obj/shaders/toneMapper.comp.spv");
	compShaderModule = createShaderModule(compShaderCode);
	compStageInfo = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eCompute, compShaderModule, "main", nullptr);

	pipelineInfo = vk::ComputePipelineCreateInfo(vk::PipelineCreateFlags(), compStageInfo, offscreenPass.toneMapper.layout, nullptr, -1);

	res = device.createComputePipelines(nullptr, 1, &pipelineInfo, nullptr, &offscreenPass.toneMapper.pipeline);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateComputePipelines Error: ") + vk::to_string(res));
	}

	device.destroyShaderModule(compShaderModule, nullptr);
}

static void createCommandPools() {
	// Offscreen Pass

	vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer), queueIndices.computeFamily);

	vk::Result res = device.createCommandPool(&poolInfo, nullptr, &offscreenPass.pool);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateCommandPool Error: ") + vk::to_string(res));
	}

	// Transfer Pass

	poolInfo = vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits(0)), queueIndices.transferFamily);

	res = device.createCommandPool(&poolInfo, nullptr, &transferPass.pool);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateCommandPool Error: ") + vk::to_string(res));
	}
}

static void createRenderCommandBuffer() {
	vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo(offscreenPass.pool, vk::CommandBufferLevel::ePrimary, 1);

	vk::Result res = device.allocateCommandBuffers(&allocInfo, &offscreenPass.buffer);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateCommandBuffers Error: ") + vk::to_string(res));
	}
}

static void recordRenderCommandBuffer() {
	vk::CommandBufferBeginInfo commandBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse, nullptr);

	offscreenPass.buffer.begin(&commandBeginInfo);

	offscreenPass.buffer.bindPipeline(vk::PipelineBindPoint::eCompute, offscreenPass.voxelUpdater.pipeline);
	offscreenPass.buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, offscreenPass.voxelUpdater.layout, 0, 1, &offscreenPass.voxelUpdater.descriptor.set, 0, nullptr);
	offscreenPass.buffer.dispatch(1, 1, 1);

	offscreenPass.buffer.bindPipeline(vk::PipelineBindPoint::eCompute, offscreenPass.rayTracer.pipeline);
	offscreenPass.buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, offscreenPass.rayTracer.layout, 0, 1, &offscreenPass.rayTracer.descriptor.set, 0, nullptr);
	offscreenPass.buffer.pushConstants(offscreenPass.rayTracer.layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(offscreenPass.rayTracer.constants), &offscreenPass.rayTracer.constants);
	offscreenPass.buffer.dispatch(1, 1, 1);

	offscreenPass.buffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlagBits::eByRegion, {},
		{ vk::BufferMemoryBarrier(vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead, queueIndices.computeFamily, queueIndices.computeFamily,
			offscreenPass.hdrImage.image, 0, offscreenPass.hdrImage.size) }, {});

	offscreenPass.buffer.bindPipeline(vk::PipelineBindPoint::eCompute, offscreenPass.toneMapper.pipeline);
	offscreenPass.buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, offscreenPass.toneMapper.layout, 0, 1, &offscreenPass.toneMapper.descriptor.set, 0, nullptr);
	offscreenPass.buffer.dispatch(1, 1, 1);

	vk::Result res = vk::Result(vkEndCommandBuffer(offscreenPass.buffer)); // use C API to get result value

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_EndCommandBuffer Error: ") + vk::to_string(res));
	}
}

static void createTransferCommandBuffers() {
	transferPass.buffers.resize(swapChainImages.size());

	vk::CommandBufferAllocateInfo allocInfo(transferPass.pool, vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(transferPass.buffers.size()));

	vk::Result res = device.allocateCommandBuffers(&allocInfo, transferPass.buffers.data());

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateCommandBuffers Error: ") + vk::to_string(res));
	}

	vk::CommandBufferBeginInfo commandBeginInfo;

	vk::ImageSubresourceLayers offsetLayer(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
	std::array<vk::Offset3D, 2> offsetExtent = { vk::Offset3D(0, 0, 0), vk::Offset3D(offscreenPass.ldrImage.extent.width, offscreenPass.ldrImage.extent.height, 1) };

	for (size_t i = 0; i < transferPass.buffers.size(); i++) {
		commandBeginInfo = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse, nullptr);
		transferPass.buffers[i].begin(&commandBeginInfo);

		transferPass.buffers[i].pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlagBits::eByRegion, {}, {},
			{ vk::ImageMemoryBarrier(vk::AccessFlagBits::eColorAttachmentRead, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
				queueIndices.presentFamily, queueIndices.transferFamily, swapChainImages[i], vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)) });

		vk::ImageSubresourceLayers imageLayer = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
		std::array<vk::Offset3D, 2> imageExtent = { vk::Offset3D(0, 0, 0), vk::Offset3D(swapChainExtent.width, swapChainExtent.height, 1) };

		transferPass.buffers[i].blitImage(offscreenPass.ldrImage.image, vk::ImageLayout::eTransferSrcOptimal, swapChainImages[i], vk::ImageLayout::eTransferDstOptimal,
			std::array<vk::ImageBlit, 1>({ vk::ImageBlit(offsetLayer, offsetExtent, imageLayer, imageExtent) }), vk::Filter::eNearest);

		transferPass.buffers[i].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlagBits::eByRegion, {}, {},
			{ vk::ImageMemoryBarrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eColorAttachmentRead, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR,
				queueIndices.transferFamily, queueIndices.presentFamily, swapChainImages[i], vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)) });

		res = vk::Result(vkEndCommandBuffer(transferPass.buffers[i])); // use C API to get result value

		if (res != vk::Result::eSuccess) {
			throw std::runtime_error(std::string("VK_EndCommandBuffer Error: ") + vk::to_string(res));
		}
	}
}

static void createSemaphores() {
	vk::SemaphoreCreateInfo semaphoreInfo = vk::SemaphoreCreateInfo(vk::SemaphoreCreateFlags());

	vk::Result res = device.createSemaphore(&semaphoreInfo, nullptr, &imageAvailable);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateSemaphore Error: ") + vk::to_string(res));
	}

	res = device.createSemaphore(&semaphoreInfo, nullptr, &renderFinished);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateSemaphore Error: ") + vk::to_string(res));
	}

	res = device.createSemaphore(&semaphoreInfo, nullptr, &offscreenPass.semaphore);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateSemaphore Error: ") + vk::to_string(res));
	}
}

static void recreateSwapChain() {
	device.waitIdle();

	vk::SurfaceCapabilitiesKHR surfaceCapabilities = physical.getSurfaceCapabilitiesKHR(surface);

	if (surfaceCapabilities.currentExtent.width == 0 || surfaceCapabilities.currentExtent.height == 0) {
		return;
	}

	cleanupSwapChain();

	createSwapChain();

	createTransferCommandBuffers();
}

static void cleanupSwapChain() {
	device.freeCommandBuffers(transferPass.pool, transferPass.buffers);

	device.destroySwapchainKHR(swapChain, nullptr);
}