#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

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

#include "operations.hpp"
#include "camera.hpp"

#define VOXEL_DEPTH 16

uint32_t tmp = (127 - VOXEL_DEPTH) << 23;

const uint16_t WIDTH = 640;
const uint16_t HEIGHT = 360;
const uint8_t  FOV = 70;
const float SCALE = *((float*)(&tmp));

const bool VSYNC = true;

SDL_Window* window;

bool isFullscreen = false;

struct Mouse {
	glm::uvec2 pos;

	bool isDragging;
} mouse;

struct Vulkan {
	vk::Instance instance;
	vk::DebugReportCallbackEXT callback;

	vk::SurfaceKHR surface;

	vk::PhysicalDevice physical;
	vk::Device device;

	std::set<uint32_t> uniqueFamilies;

	vk::PhysicalDeviceMemoryProperties memoryProperties;

	vk::PhysicalDeviceSubgroupProperties subgroupProperties;

#ifdef NDEBUG
	const std::vector<const char*> layers = {};
#else
	const std::vector<const char*> layers = {
		"VK_LAYER_LUNARG_standard_validation"
	};
#endif

	const std::vector<const char*> extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_EXT_SHADER_SUBGROUP_BALLOT_EXTENSION_NAME,
		VK_EXT_SHADER_SUBGROUP_VOTE_EXTENSION_NAME
	};
} vulkan;

struct FeedbackPass {
	uint32_t family;
	vk::Queue queue;

	vk::CommandPool pool;
	std::pair<vk::CommandBuffer, vk::CommandBuffer> primary;

	vk::Fence download;

	vk::DeviceSize size = voxels::size;

	struct CPUStaging {
		vk::Buffer buffer;
		vk::DeviceMemory memory;

		uint8_t* temporary;
	} cpuStaging;

	struct GPUStaging {
		vk::Buffer buffer;
		vk::DeviceMemory memory;
	} gpuStaging;

	struct WorkingSet {
		uint8_t* update = voxels::content;
		uint8_t visibility[voxels::size];
	} workingSet;
} feedbackPass;

struct RenderPass {
	uint32_t family;
	vk::Queue queue;

	vk::CommandPool pool;
	std::pair<vk::CommandBuffer, vk::CommandBuffer> primary;

	std::pair<vk::Semaphore, vk::Semaphore> render;
	vk::Semaphore finished;

	struct HDRImage {
		vk::Format format = vk::Format::eR16G16B16A16Sfloat;
		vk::DeviceSize size = sizeof(glm::i16vec4) * WIDTH * HEIGHT;

		vk::Buffer image;
		vk::DeviceMemory memory;
		vk::BufferView view;
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

		vk::CommandBuffer secondary;

		struct Descriptor {
			vk::DescriptorPool pool;
			vk::DescriptorSetLayout layout;
			vk::DescriptorSet set;
		} descriptor;
	} voxelUpdater;

	struct RayTracer {
		vk::PipelineLayout layout;
		vk::Pipeline pipeline;

		vk::CommandBuffer secondary;

		struct Descriptor {
			vk::DescriptorPool pool;
			vk::DescriptorSetLayout layout;
			vk::DescriptorSet set;
		} descriptor;

		struct Queue {
			vk::DeviceSize size = sizeof(glm::uint);

			vk::Buffer buffer;
			vk::DeviceMemory memory;
		} queue;

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

		vk::CommandBuffer secondary;

		struct Descriptor {
			vk::DescriptorPool pool;
			vk::DescriptorSetLayout layout;
			vk::DescriptorSet set;
		} descriptor;
	} toneMapper;
} renderPass;

struct PresentationPass {
	uint32_t family;
	vk::Queue queue;

	vk::CommandPool pool;
	std::vector<vk::CommandBuffer> primary;

	vk::Semaphore available;
	vk::Semaphore finished;

	struct SwapChain {
		vk::SwapchainKHR queue;

		std::vector<vk::Image> images;

		vk::Format format;
		vk::Extent2D extent;

		vk::Viewport viewport;
	} swapChain;
} presentationPass;

struct VoxelBuffer {
	vk::Extent3D extent = vk::Extent3D(256, 256, 256);

	struct Structure {
		vk::Format format = vk::Format::eR32Uint;

		vk::Image image;
		vk::DeviceMemory memory;
		vk::ImageView view;
	} structure;

	struct Material {
		vk::Format format = vk::Format::eR32G32B32A32Uint;

		vk::Image image;
		vk::DeviceMemory memory;
		vk::ImageView view;
	} material;
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
static void initVoxelBuffer();
static void initRenderer();
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
static void createBufferViews();

static void createDescriptorPools();
static void createDescriptorSets();
static void createRenderPipelines();

static void createSemaphores();
static void createFences();

static void createCommandPools();
static void createFeedbackCommandBuffer();
static void createRenderCommandBuffers();
static void recordRenderCommandBuffers(bool voxelUpdater, bool rayTracer);
static void createPresentationCommandBuffers();

static void recreateSwapChain();
static void cleanupSwapChain();

int SDL_main(int argc, char* argv[]) {
	initState();
	
	try {
		initSDL();
		initVulkan();

		initVoxelBuffer();
		initRenderer();

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
	createBufferViews();

	createDescriptorPools();
	createDescriptorSets();
	createRenderPipelines();

	createSemaphores();
	createFences();

	createCommandPools();
	createFeedbackCommandBuffer();
	createRenderCommandBuffers();
	recordRenderCommandBuffers(true, true);
	createPresentationCommandBuffers();
}

static void update() {
	uint32_t width, height, size;

	SDL_Event event;
	
	while (true) {
		SDL_GetWindowSize(window, (int*)&width, (int*)&height);
		size = height ^ ((width ^ height) & -(width < height));

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				return;
			}
			
			if (event.type == SDL_WINDOWEVENT) {
				vk::SurfaceCapabilitiesKHR surfaceCapabilities = vulkan.physical.getSurfaceCapabilitiesKHR(vulkan.surface);

				if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() && surfaceCapabilities.currentExtent.height != std::numeric_limits<uint32_t>::max()) {
					presentationPass.swapChain.extent = surfaceCapabilities.currentExtent;
				}
				else {
					presentationPass.swapChain.extent = vk::Extent2D(std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, width)),
						std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, height)));
				}
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_ESCAPE) {
					if (!isFullscreen) {
						return;
					}
					
					changeFullscreen(false);
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

		if (presentationPass.swapChain.extent.width > 0 && presentationPass.swapChain.extent.height > 0) {
			updateState();

			drawFrame();
		}
	}
}

static void cleanup() {
	vulkan.device.waitIdle();

	vulkan.device.destroySemaphore(renderPass.render.first, nullptr);
	vulkan.device.destroySemaphore(renderPass.render.second, nullptr);
	vulkan.device.destroySemaphore(renderPass.finished, nullptr);
	vulkan.device.destroySemaphore(presentationPass.available, nullptr);
	vulkan.device.destroySemaphore(presentationPass.finished, nullptr);

	vulkan.device.destroyFence(feedbackPass.download, nullptr);

	vulkan.device.freeCommandBuffers(feedbackPass.pool, { feedbackPass.primary.first, feedbackPass.primary.second });
	vulkan.device.destroyCommandPool(feedbackPass.pool, nullptr);
	vulkan.device.freeCommandBuffers(renderPass.pool, {
		renderPass.primary.first, renderPass.voxelUpdater.secondary, renderPass.rayTracer.secondary,
		renderPass.primary.second, renderPass.toneMapper.secondary
	});
	vulkan.device.destroyCommandPool(renderPass.pool, nullptr);
	vulkan.device.freeCommandBuffers(presentationPass.pool, presentationPass.primary);
	vulkan.device.destroyCommandPool(presentationPass.pool, nullptr);

	vulkan.device.destroyPipeline(renderPass.voxelUpdater.pipeline, nullptr);
	vulkan.device.destroyPipelineLayout(renderPass.voxelUpdater.layout, nullptr);
	vulkan.device.destroyPipeline(renderPass.rayTracer.pipeline, nullptr);
	vulkan.device.destroyPipelineLayout(renderPass.rayTracer.layout, nullptr);
	vulkan.device.destroyPipeline(renderPass.toneMapper.pipeline, nullptr);
	vulkan.device.destroyPipelineLayout(renderPass.toneMapper.layout, nullptr);

	vulkan.device.destroyDescriptorSetLayout(renderPass.voxelUpdater.descriptor.layout, nullptr);
	vulkan.device.destroyDescriptorPool(renderPass.voxelUpdater.descriptor.pool, nullptr);
	vulkan.device.destroyDescriptorSetLayout(renderPass.rayTracer.descriptor.layout, nullptr);
	vulkan.device.destroyDescriptorPool(renderPass.rayTracer.descriptor.pool, nullptr);
	vulkan.device.destroyDescriptorSetLayout(renderPass.toneMapper.descriptor.layout, nullptr);
	vulkan.device.destroyDescriptorPool(renderPass.toneMapper.descriptor.pool, nullptr);

	vulkan.device.unmapMemory(feedbackPass.cpuStaging.memory);

	vulkan.device.destroyBuffer(feedbackPass.cpuStaging.buffer, nullptr);
	vulkan.device.freeMemory(feedbackPass.cpuStaging.memory, nullptr);
	vulkan.device.destroyBuffer(feedbackPass.gpuStaging.buffer, nullptr);
	vulkan.device.freeMemory(feedbackPass.gpuStaging.memory, nullptr);
	vulkan.device.destroyBufferView(renderPass.hdrImage.view, nullptr);
	vulkan.device.destroyBuffer(renderPass.hdrImage.image, nullptr);
	vulkan.device.freeMemory(renderPass.hdrImage.memory, nullptr);
	vulkan.device.destroyBuffer(renderPass.rayTracer.queue.buffer, nullptr);
	vulkan.device.freeMemory(renderPass.rayTracer.queue.memory, nullptr);

	vulkan.device.destroyImageView(renderPass.ldrImage.view, nullptr);
	vulkan.device.destroyImage(renderPass.ldrImage.image, nullptr);
	vulkan.device.freeMemory(renderPass.ldrImage.memory, nullptr);
	vulkan.device.destroyImageView(voxelBuffer.structure.view, nullptr);
	vulkan.device.destroyImage(voxelBuffer.structure.image, nullptr);
	vulkan.device.freeMemory(voxelBuffer.structure.memory, nullptr);
	vulkan.device.destroyImageView(voxelBuffer.material.view, nullptr);
	vulkan.device.destroyImage(voxelBuffer.material.image, nullptr);
	vulkan.device.freeMemory(voxelBuffer.material.memory, nullptr);

	vulkan.device.destroySwapchainKHR(presentationPass.swapChain.queue, nullptr);

	vulkan.device.destroy();

#ifndef NDEBUG
	((PFN_vkDestroyDebugReportCallbackEXT)vulkan.instance.getProcAddr("vkDestroyDebugReportCallbackEXT"))(vulkan.instance, static_cast<VkDebugReportCallbackEXT>(vulkan.callback), nullptr);
#endif

	vulkan.instance.destroySurfaceKHR(vulkan.surface);
	vulkan.instance.destroy();

	SDL_DestroyWindow(window);
	SDL_Quit();
}

static void initState() {
	camera::mov(glm::vec3(0.5f, 0.5f, -0.5f));
	camera::rad(10.0f);

	camera::upd(renderPass.rayTracer.constants.pos, renderPass.rayTracer.constants.rot);

	renderPass.rayTracer.constants.pos = renderPass.rayTracer.constants.pos * SCALE + 1.5f;
}

static void updateState() {
	if (camera::upd(renderPass.rayTracer.constants.pos, renderPass.rayTracer.constants.rot)) {
		renderPass.rayTracer.constants.pos = renderPass.rayTracer.constants.pos * SCALE + 1.5f;

		recordRenderCommandBuffers(false, true);
	}
}

static void initVoxelBuffer() {
	voxels::SubgroupSize = vulkan.subgroupProperties.subgroupSize;

	voxels::reset();

	voxels::submit(STR_LOA_S(0, 0x00));
}

static void initRenderer() {
	memcpy(feedbackPass.cpuStaging.temporary, feedbackPass.workingSet.update, feedbackPass.size);

	voxels::reset();

	vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &feedbackPass.primary.second, 0, nullptr);

	vk::Result res = feedbackPass.queue.submit(1, &submitInfo, nullptr);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_QueueSubmit Error: ") + vk::to_string(res));
	}
}

static void changeFullscreen(bool fullscreen) {
	if (fullscreen == isFullscreen) {
		return;
	}

	isFullscreen = fullscreen;

	vulkan.device.waitIdle();

	SDL_SetWindowFullscreen(window, isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);

	recreateSwapChain();
}

static void drawFrame() {
	feedbackPass.queue.waitIdle();

	uint32_t imageIndex;
	vk::Result res = vulkan.device.acquireNextImageKHR(presentationPass.swapChain.queue, std::numeric_limits<uint64_t>::max(), presentationPass.available, nullptr, &imageIndex);

	if (res == vk::Result::eErrorOutOfDateKHR) {
		recreateSwapChain();

		return;
	}
	else if (res != vk::Result::eSuccess && res != vk::Result::eSuboptimalKHR) {
		throw std::runtime_error(std::string("VK_AcquireNextImageKHR Error: ") + vk::to_string(res));
	}

	vk::PipelineStageFlags waitStages[] = {
		vk::PipelineStageFlagBits::eAllCommands,
		vk::PipelineStageFlagBits::eComputeShader
	};

	std::array<vk::Semaphore, 2> semaphores = { renderPass.render.first, renderPass.render.second };

	std::array<vk::SubmitInfo, 2> submitInfos = {
		vk::SubmitInfo(0, nullptr, nullptr, 1, &renderPass.primary.first, 2, semaphores.data()),
		vk::SubmitInfo(1, &renderPass.render.first, &waitStages[0], 1, &renderPass.primary.second, 1, &renderPass.finished)
	};

	res = renderPass.queue.submit(2, submitInfos.data(), nullptr);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_QueueSubmit Error: ") + vk::to_string(res));
	}

	semaphores = { presentationPass.available, renderPass.finished };

	submitInfos = {
		vk::SubmitInfo(1, &renderPass.render.second, &waitStages[0], 1, &feedbackPass.primary.first, 0, nullptr),
		vk::SubmitInfo(2, semaphores.data(), waitStages, 1, &presentationPass.primary[imageIndex], 1, &presentationPass.finished)
	};

	res = feedbackPass.queue.submit(1, &submitInfos[0], feedbackPass.download);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_QueueSubmit Error: ") + vk::to_string(res));
	}

	res = presentationPass.queue.submit(1, &submitInfos[1], nullptr);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_QueueSubmit Error: ") + vk::to_string(res));
	}

	vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR(1, &presentationPass.finished, 1, &presentationPass.swapChain.queue, &imageIndex, nullptr);

	res = presentationPass.queue.presentKHR(&presentInfo);

	if (res == vk::Result::eErrorOutOfDateKHR || res == vk::Result::eSuboptimalKHR) {
		recreateSwapChain();
	}
	else if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_QueuePresentKHR Error: ") + vk::to_string(res));
	}

	vulkan.device.waitForFences(1, &feedbackPass.download, true, std::numeric_limits<uint64_t>::max());
	vulkan.device.resetFences(1, &feedbackPass.download);

	memcpy(feedbackPass.workingSet.visibility, feedbackPass.cpuStaging.temporary, feedbackPass.size);
	memcpy(feedbackPass.cpuStaging.temporary, feedbackPass.workingSet.update, feedbackPass.size);
	
	// DEBUG Begin

	uint8_t* begin = feedbackPass.workingSet.visibility;
	uint8_t* pointer = begin;
	uint8_t* end = begin + feedbackPass.size;

	std::cout << "Visible voxels: ";

	while (pointer < end) {
		if (*pointer != 0x00) {

			printf("%u(0x%02X) ", uint32_t(pointer - begin), *pointer);
		}

		pointer++;
	}

	std::cout << std::endl;

	// DEBUG End

	voxels::reset();

	submitInfos[1] = vk::SubmitInfo(0, nullptr, nullptr, 1, &feedbackPass.primary.second, 0, nullptr);

	res = feedbackPass.queue.submit(1, &submitInfos[1], nullptr);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_QueueSubmit Error: ") + vk::to_string(res));
	}

	presentationPass.queue.waitIdle();
}

static void createInstance() {
	vk::ApplicationInfo appInfo = vk::ApplicationInfo("Voxel Engine", VK_MAKE_VERSION(1, 1, 0), "LunarG SDK", VK_MAKE_VERSION(1, 1, 0), VK_API_VERSION_1_1);

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

	vk::InstanceCreateInfo instanceInfo(vk::InstanceCreateFlags(), &appInfo, static_cast<uint32_t>(vulkan.layers.size()), vulkan.layers.data(), static_cast<uint32_t>(instExt.size()), instExt.data());

	vk::Result res = vk::createInstance(&instanceInfo, nullptr, &vulkan.instance);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateInstance Error: ") + vk::to_string(res));
	}
}

static void createDebugCallback() {
#ifndef NDEBUG
	vk::DebugReportCallbackCreateInfoEXT callInfo(vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning, debugCallback);

	vk::Result res = vk::Result(((PFN_vkCreateDebugReportCallbackEXT)vulkan.instance.getProcAddr("vkCreateDebugReportCallbackEXT"))(vulkan.instance,
		reinterpret_cast<const VkDebugReportCallbackCreateInfoEXT*>(&callInfo), nullptr, reinterpret_cast<VkDebugReportCallbackEXT*>(&vulkan.callback)));

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDebugReportCallbackEXT Error: ") + vk::to_string(res));
	}
#endif
}

static void createSurface() {
	if (SDL_Vulkan_CreateSurface(window, vulkan.instance, reinterpret_cast<VkSurfaceKHR*>(&vulkan.surface)) != SDL_TRUE) {
		throw std::runtime_error(std::string("SDL_Vulkan_CreateSurface Error: ") + SDL_GetError());
	}
}

static void pickPhysicalDevice() {
	std::vector<vk::PhysicalDevice> physDevs = vulkan.instance.enumeratePhysicalDevices();

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

					prsQueIdx = dev.getSurfaceSupportKHR(tmp, vulkan.surface) ? tmp : -1;
				}
				
				if ((queFamily.queueFlags & vk::QueueFlagBits::eTransfer) == vk::QueueFlagBits::eTransfer) {
					traQueIdx = tmp;
				}
			}

			tmp++;
		}

		if (comQueIdx == -1 || traQueIdx == -1 || prsQueIdx == -1) {
			continue;
		}

		avaExt = dev.enumerateDeviceExtensionProperties(nullptr);

		reqExt = std::set<std::string>(vulkan.extensions.begin(), vulkan.extensions.end());

		for (const vk::ExtensionProperties ext : avaExt) {
			reqExt.erase(ext.extensionName);
		}

		if (!reqExt.empty()) {
			continue;
		}

		if (dev.getSurfaceFormatsKHR(vulkan.surface).empty() || dev.getSurfacePresentModesKHR(vulkan.surface).empty()) {
			continue;
		}

		if ((dev.getSurfaceCapabilitiesKHR(vulkan.surface).supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst) != vk::ImageUsageFlagBits::eTransferDst) {
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

	vulkan.physical = selDev;

	vulkan.physical.getMemoryProperties(&vulkan.memoryProperties);

	vk::PhysicalDeviceProperties2 devicePropertiesQuery;
	devicePropertiesQuery.pNext = &vulkan.subgroupProperties;

	vulkan.physical.getProperties2(&devicePropertiesQuery);
	
	feedbackPass.family = transferQueueIndex;
	renderPass.family = computeQueueIndex;
	presentationPass.family = presentationQueueIndex;

	vulkan.uniqueFamilies = { computeQueueIndex, transferQueueIndex, presentationQueueIndex };
}

static void createLogicalDevice() {
	std::vector<vk::DeviceQueueCreateInfo> queueInfos;

	float queuePriority = 1.0f;

	for (const uint32_t queFam : vulkan.uniqueFamilies) {
		queueInfos.push_back(vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), queFam, 1, &queuePriority));
	}

	vk::PhysicalDeviceFeatures deviceFeatures;
	deviceFeatures.shaderStorageImageExtendedFormats = true;
	deviceFeatures.shaderInt64 = true;

	vk::DeviceCreateInfo deviceInfo(vk::DeviceCreateFlags(), static_cast<uint32_t>(queueInfos.size()), queueInfos.data(), static_cast<uint32_t>(vulkan.layers.size()),
		vulkan.layers.data(), static_cast<uint32_t>(vulkan.extensions.size()), vulkan.extensions.data(), &deviceFeatures);

	vk::Result res = vulkan.physical.createDevice(&deviceInfo, nullptr, &vulkan.device);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDevice Error: ") + vk::to_string(res));
	}

	feedbackPass.queue = vulkan.device.getQueue(feedbackPass.family, 0);
	renderPass.queue = vulkan.device.getQueue(renderPass.family, 0);
	presentationPass.queue = vulkan.device.getQueue(presentationPass.family, 0);
}

static void createSwapChain() {
	vk::SurfaceCapabilitiesKHR surfaceCapabilities = vulkan.physical.getSurfaceCapabilitiesKHR(vulkan.surface);
	std::vector<vk::SurfaceFormatKHR> surfaceFormats = vulkan.physical.getSurfaceFormatsKHR(vulkan.surface);
	std::vector<vk::PresentModeKHR> presentModes = vulkan.physical.getSurfacePresentModesKHR(vulkan.surface);

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

	presentationPass.swapChain.format = surfaceFormat.format;

	vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;

	if (!VSYNC) {
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
	}

	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() && surfaceCapabilities.currentExtent.height != std::numeric_limits<uint32_t>::max()) {
		presentationPass.swapChain.extent = surfaceCapabilities.currentExtent;
	}
	else {
		uint32_t width, height;

		SDL_GetWindowSize(window, (int*)&width, (int*)&height);

		presentationPass.swapChain.extent = vk::Extent2D(std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, width)),
			std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, height)));
	}

	uint32_t imageCount = surfaceCapabilities.minImageCount + 1;

	if (surfaceCapabilities.maxImageCount > 0 && imageCount > surfaceCapabilities.maxImageCount) {
		imageCount = surfaceCapabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR swapInfo;
	swapInfo.surface = vulkan.surface;
	swapInfo.minImageCount = imageCount;
	swapInfo.imageFormat = surfaceFormat.format;
	swapInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapInfo.imageExtent = presentationPass.swapChain.extent;
	swapInfo.imageArrayLayers = 1;
	swapInfo.imageUsage = vk::ImageUsageFlagBits::eTransferDst;
	swapInfo.imageSharingMode = vk::SharingMode::eExclusive;
	swapInfo.queueFamilyIndexCount = 0;
	swapInfo.pQueueFamilyIndices = nullptr;
	swapInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
	swapInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	swapInfo.presentMode = presentMode;
	swapInfo.clipped = false;
	swapInfo.oldSwapchain = nullptr;

	vk::Result res = vulkan.device.createSwapchainKHR(&swapInfo, nullptr, &presentationPass.swapChain.queue);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateSwapchainKHR Error: ") + vk::to_string(res));
	}

	presentationPass.swapChain.images = vulkan.device.getSwapchainImagesKHR(presentationPass.swapChain.queue);
}

static uint32_t getMemoryTypeIndex(uint32_t typeBits, vk::MemoryPropertyFlags flags) {
	uint32_t index = -1;

	for (uint32_t i = 0; i < vulkan.memoryProperties.memoryTypeCount; i++) {
		if ((typeBits & 0x01) == 0x01) {
			if ((vulkan.memoryProperties.memoryTypes[i].propertyFlags & flags) == flags) {
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

	vk::ImageCreateInfo imageInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, renderPass.ldrImage.format, renderPass.ldrImage.extent, 1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined);

	vk::Result res = vulkan.device.createImage(&imageInfo, nullptr, &renderPass.ldrImage.image);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateImage Error: ") + vk::to_string(res));
	}

	vk::MemoryRequirements memReqs = vulkan.device.getImageMemoryRequirements(renderPass.ldrImage.image);

	vk::MemoryAllocateInfo allocInfo(memReqs.size, getMemoryTypeIndex(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

	res = vulkan.device.allocateMemory(&allocInfo, nullptr, &renderPass.ldrImage.memory);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateMemory Error: ") + vk::to_string(res));
	}

	vulkan.device.bindImageMemory(renderPass.ldrImage.image, renderPass.ldrImage.memory, 0);

	// Voxel structure

	imageInfo = vk::ImageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e3D, voxelBuffer.structure.format, voxelBuffer.extent, 1, 1, vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eStorage, vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined);

	res = vulkan.device.createImage(&imageInfo, nullptr, &voxelBuffer.structure.image);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateImage Error: ") + vk::to_string(res));
	}

	memReqs = vulkan.device.getImageMemoryRequirements(voxelBuffer.structure.image);
	
	allocInfo = vk::MemoryAllocateInfo(memReqs.size, getMemoryTypeIndex(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

	res = vulkan.device.allocateMemory(&allocInfo, nullptr, &voxelBuffer.structure.memory);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateMemory Error: ") + vk::to_string(res));
	}

	vulkan.device.bindImageMemory(voxelBuffer.structure.image, voxelBuffer.structure.memory, 0);

	// Voxel material

	imageInfo = vk::ImageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e3D, voxelBuffer.material.format, voxelBuffer.extent, 1, 1, vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eStorage, vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined);

	res = vulkan.device.createImage(&imageInfo, nullptr, &voxelBuffer.material.image);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateImage Error: ") + vk::to_string(res));
	}

	memReqs = vulkan.device.getImageMemoryRequirements(voxelBuffer.material.image);

	allocInfo = vk::MemoryAllocateInfo(memReqs.size, getMemoryTypeIndex(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

	res = vulkan.device.allocateMemory(&allocInfo, nullptr, &voxelBuffer.material.memory);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateMemory Error: ") + vk::to_string(res));
	}

	vulkan.device.bindImageMemory(voxelBuffer.material.image, voxelBuffer.material.memory, 0);
}

static void createImageViews() {
	// LDR Image

	vk::ImageViewCreateInfo viewInfo(vk::ImageViewCreateFlags(), renderPass.ldrImage.image, vk::ImageViewType::e2D, renderPass.ldrImage.format, vk::ComponentMapping(vk::ComponentSwizzle::eIdentity,
		vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

	vk::Result res = vulkan.device.createImageView(&viewInfo, nullptr, &renderPass.ldrImage.view);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateImageView Error: ") + vk::to_string(res));
	}

	// Voxel structure

	viewInfo = vk::ImageViewCreateInfo(vk::ImageViewCreateFlags(), voxelBuffer.structure.image, vk::ImageViewType::e3D, voxelBuffer.structure.format, vk::ComponentMapping(vk::ComponentSwizzle::eIdentity,
		vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

	res = vulkan.device.createImageView(&viewInfo, nullptr, &voxelBuffer.structure.view);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateImageView Error: ") + vk::to_string(res));
	}

	// Voxel material

	viewInfo = vk::ImageViewCreateInfo(vk::ImageViewCreateFlags(), voxelBuffer.material.image, vk::ImageViewType::e3D, voxelBuffer.material.format, vk::ComponentMapping(vk::ComponentSwizzle::eIdentity,
		vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

	res = vulkan.device.createImageView(&viewInfo, nullptr, &voxelBuffer.material.view);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateImageView Error: ") + vk::to_string(res));
	}
}

static void createBuffers() {
	// CPU Staging

	vk::BufferCreateInfo bufferInfo(vk::BufferCreateFlags(), feedbackPass.size, vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eStorageBuffer |
		vk::BufferUsageFlagBits::eTransferDst, vk::SharingMode::eExclusive, 0, nullptr);

	vk::Result res = vulkan.device.createBuffer(&bufferInfo, nullptr, &feedbackPass.cpuStaging.buffer);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateBuffer Error: ") + vk::to_string(res));
	}

	vk::MemoryRequirements memReqs = vulkan.device.getBufferMemoryRequirements(feedbackPass.cpuStaging.buffer);

	vk::MemoryAllocateInfo allocInfo(memReqs.size, getMemoryTypeIndex(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent));

	res = vulkan.device.allocateMemory(&allocInfo, nullptr, &feedbackPass.cpuStaging.memory);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateMemory Error: ") + vk::to_string(res));
	}

	vulkan.device.bindBufferMemory(feedbackPass.cpuStaging.buffer, feedbackPass.cpuStaging.memory, 0);
	vulkan.device.mapMemory(feedbackPass.cpuStaging.memory, 0, feedbackPass.size, vk::MemoryMapFlags(), (void**)&feedbackPass.cpuStaging.temporary);

	// GPU Staging

	bufferInfo = vk::BufferCreateInfo(vk::BufferCreateFlags(), feedbackPass.size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eStorageBuffer |
		vk::BufferUsageFlagBits::eTransferSrc, vk::SharingMode::eExclusive, 0, nullptr);

	res = vulkan.device.createBuffer(&bufferInfo, nullptr, &feedbackPass.gpuStaging.buffer);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateBuffer Error: ") + vk::to_string(res));
	}

	memReqs = vulkan.device.getBufferMemoryRequirements(feedbackPass.gpuStaging.buffer);

	allocInfo = vk::MemoryAllocateInfo(memReqs.size, getMemoryTypeIndex(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

	res = vulkan.device.allocateMemory(&allocInfo, nullptr, &feedbackPass.gpuStaging.memory);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateMemory Error: ") + vk::to_string(res));
	}

	vulkan.device.bindBufferMemory(feedbackPass.gpuStaging.buffer, feedbackPass.gpuStaging.memory, 0);

	// HDR Image

	bufferInfo = vk::BufferCreateInfo(vk::BufferCreateFlags(), renderPass.hdrImage.size, vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eStorageTexelBuffer,
		vk::SharingMode::eExclusive, 0, nullptr);

	res = vulkan.device.createBuffer(&bufferInfo, nullptr, &renderPass.hdrImage.image);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateBuffer Error: ") + vk::to_string(res));
	}

	memReqs = vulkan.device.getBufferMemoryRequirements(renderPass.hdrImage.image);

	allocInfo = vk::MemoryAllocateInfo(memReqs.size, getMemoryTypeIndex(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

	res = vulkan.device.allocateMemory(&allocInfo, nullptr, &renderPass.hdrImage.memory);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateMemory Error: ") + vk::to_string(res));
	}

	vulkan.device.bindBufferMemory(renderPass.hdrImage.image, renderPass.hdrImage.memory, 0);

	// Ray Queue

	bufferInfo = vk::BufferCreateInfo(vk::BufferCreateFlags(), renderPass.rayTracer.queue.size, vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive, 0, nullptr);

	res = vulkan.device.createBuffer(&bufferInfo, nullptr, &renderPass.rayTracer.queue.buffer);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateBuffer Error: ") + vk::to_string(res));
	}

	memReqs = vulkan.device.getBufferMemoryRequirements(renderPass.rayTracer.queue.buffer);

	allocInfo = vk::MemoryAllocateInfo(memReqs.size, getMemoryTypeIndex(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

	res = vulkan.device.allocateMemory(&allocInfo, nullptr, &renderPass.rayTracer.queue.memory);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateMemory Error: ") + vk::to_string(res));
	}

	vulkan.device.bindBufferMemory(renderPass.rayTracer.queue.buffer, renderPass.rayTracer.queue.memory, 0);
}

static void createBufferViews() {
	// HDR Image

	vk::BufferViewCreateInfo viewInfo(vk::BufferViewCreateFlags(), renderPass.hdrImage.image, renderPass.hdrImage.format, 0, renderPass.hdrImage.size);

	vk::Result res = vulkan.device.createBufferView(&viewInfo, nullptr, &renderPass.hdrImage.view);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateBufferView Error: ") + vk::to_string(res));
	}
}

static void createDescriptorPools() {
	// Voxel Updater

	std::vector<vk::DescriptorPoolSize> poolSizes = {
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 2)
	};

	vk::DescriptorPoolCreateInfo poolInfo(vk::DescriptorPoolCreateFlags(), 1, static_cast<uint32_t>(poolSizes.size()), poolSizes.data());

	vk::Result res = vulkan.device.createDescriptorPool(&poolInfo, nullptr, &renderPass.voxelUpdater.descriptor.pool);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDescriptorPool Error: ") + vk::to_string(res));
	}

	// Ray Tracer

	poolSizes = {
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 2),
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 3)
	};

	poolInfo = vk::DescriptorPoolCreateInfo(vk::DescriptorPoolCreateFlags(), 1, static_cast<uint32_t>(poolSizes.size()), poolSizes.data());

	res = vulkan.device.createDescriptorPool(&poolInfo, nullptr, &renderPass.rayTracer.descriptor.pool);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDescriptorPool Error: ") + vk::to_string(res));
	}

	// Tone Mapper

	poolSizes = {
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1),
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, 1)
	};

	poolInfo = vk::DescriptorPoolCreateInfo(vk::DescriptorPoolCreateFlags(), 1, static_cast<uint32_t>(poolSizes.size()), poolSizes.data());

	res = vulkan.device.createDescriptorPool(&poolInfo, nullptr, &renderPass.toneMapper.descriptor.pool);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDescriptorPool Error: ") + vk::to_string(res));
	}
}

static void createDescriptorSets() {
	// Voxel Updater

	std::vector<vk::DescriptorSetLayoutBinding> bindings = {
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
		vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
		vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr)
	};

	vk::DescriptorSetLayoutCreateInfo layoutInfo(vk::DescriptorSetLayoutCreateFlags(), static_cast<uint32_t>(bindings.size()), bindings.data());

	vk::Result res = vulkan.device.createDescriptorSetLayout(&layoutInfo, nullptr, &renderPass.voxelUpdater.descriptor.layout);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDescriptorSetLayout Error: ") + vk::to_string(res));
	}

	vk::DescriptorSetAllocateInfo allocInfo(renderPass.voxelUpdater.descriptor.pool, 1, &renderPass.voxelUpdater.descriptor.layout);

	res = vulkan.device.allocateDescriptorSets(&allocInfo, &renderPass.voxelUpdater.descriptor.set);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateDescriptorSets Error: ") + vk::to_string(res));
	}

	// Ray Tracer

	bindings = {
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
		vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
		vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
		vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr)
	};

	layoutInfo = vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags(), static_cast<uint32_t>(bindings.size()), bindings.data());

	res = vulkan.device.createDescriptorSetLayout(&layoutInfo, nullptr, &renderPass.rayTracer.descriptor.layout);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDescriptorSetLayout Error: ") + vk::to_string(res));
	}

	allocInfo = vk::DescriptorSetAllocateInfo(renderPass.rayTracer.descriptor.pool, 1, &renderPass.rayTracer.descriptor.layout);

	res = vulkan.device.allocateDescriptorSets(&allocInfo, &renderPass.rayTracer.descriptor.set);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateDescriptorSets Error: ") + vk::to_string(res));
	}

	// Tone Mapper

	bindings = {
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageTexelBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr)
	};

	layoutInfo = vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags(), static_cast<uint32_t>(bindings.size()), bindings.data());

	res = vulkan.device.createDescriptorSetLayout(&layoutInfo, nullptr, &renderPass.toneMapper.descriptor.layout);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDescriptorSetLayout Error: ") + vk::to_string(res));
	}

	allocInfo = vk::DescriptorSetAllocateInfo(renderPass.toneMapper.descriptor.pool, 1, &renderPass.toneMapper.descriptor.layout);

	res = vulkan.device.allocateDescriptorSets(&allocInfo, &renderPass.toneMapper.descriptor.set);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateDescriptorSets Error: ") + vk::to_string(res));
	}

	// Write all bindings

	vk::DescriptorBufferInfo fedInfo(feedbackPass.gpuStaging.buffer, 0, feedbackPass.size);
	vk::DescriptorImageInfo strInfo(nullptr, voxelBuffer.structure.view, vk::ImageLayout::eGeneral);
	vk::DescriptorImageInfo matInfo(nullptr, voxelBuffer.material.view, vk::ImageLayout::eGeneral);
	vk::DescriptorBufferInfo rayInfo(renderPass.rayTracer.queue.buffer, 0, renderPass.rayTracer.queue.size);
	vk::DescriptorBufferInfo hdrInfo(renderPass.hdrImage.image, 0, renderPass.hdrImage.size);
	vk::DescriptorImageInfo ldrInfo(nullptr, renderPass.ldrImage.view, vk::ImageLayout::eGeneral);

	std::vector<vk::WriteDescriptorSet> descriptorWrites = {
		vk::WriteDescriptorSet(renderPass.voxelUpdater.descriptor.set, 0, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &fedInfo, nullptr),
		vk::WriteDescriptorSet(renderPass.voxelUpdater.descriptor.set, 1, 0, 1, vk::DescriptorType::eStorageImage, &strInfo, nullptr, nullptr),
		vk::WriteDescriptorSet(renderPass.voxelUpdater.descriptor.set, 2, 0, 1, vk::DescriptorType::eStorageImage, &matInfo, nullptr, nullptr),
		vk::WriteDescriptorSet(renderPass.rayTracer.descriptor.set, 0, 0, 1, vk::DescriptorType::eStorageImage, &strInfo, nullptr, nullptr),
		vk::WriteDescriptorSet(renderPass.rayTracer.descriptor.set, 1, 0, 1, vk::DescriptorType::eStorageImage, &matInfo, nullptr, nullptr),
		vk::WriteDescriptorSet(renderPass.rayTracer.descriptor.set, 2, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &rayInfo, nullptr),
		vk::WriteDescriptorSet(renderPass.rayTracer.descriptor.set, 3, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &fedInfo, nullptr),
		vk::WriteDescriptorSet(renderPass.rayTracer.descriptor.set, 4, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &hdrInfo, nullptr),
		vk::WriteDescriptorSet(renderPass.toneMapper.descriptor.set, 0, 0, 1, vk::DescriptorType::eStorageTexelBuffer, nullptr, nullptr, &renderPass.hdrImage.view),
		vk::WriteDescriptorSet(renderPass.toneMapper.descriptor.set, 1, 0, 1, vk::DescriptorType::eStorageImage, &ldrInfo, nullptr, nullptr)
	};

	vulkan.device.updateDescriptorSets(descriptorWrites, nullptr);
}

static vk::ShaderModule createShaderModule(const std::vector<char>& code) {
	vk::ShaderModuleCreateInfo moduleInfo(vk::ShaderModuleCreateFlags(), code.size(), reinterpret_cast<const uint32_t*>(code.data()));

	vk::ShaderModule shaderModule;

	vk::Result res = vulkan.device.createShaderModule(&moduleInfo, nullptr, &shaderModule);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateShaderModule Error: ") + vk::to_string(res));
	}

	return shaderModule;
}

static void createRenderPipelines() {
	// Voxel Updater

	vk::PipelineLayoutCreateInfo layoutInfo(vk::PipelineLayoutCreateFlags(), 1, &renderPass.voxelUpdater.descriptor.layout, 0, nullptr);

	vk::Result res = vulkan.device.createPipelineLayout(&layoutInfo, nullptr, &renderPass.voxelUpdater.layout);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreatePipelineLayout Error: ") + vk::to_string(res));
	}

	std::vector<char> compShaderCode = readFile("obj/shaders/voxelUpdater.comp.spv");
	vk::ShaderModule compShaderModule = createShaderModule(compShaderCode);
	vk::PipelineShaderStageCreateInfo compStageInfo(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eCompute, compShaderModule, "main", nullptr);

	vk::ComputePipelineCreateInfo pipelineInfo(vk::PipelineCreateFlags(), compStageInfo, renderPass.voxelUpdater.layout, nullptr, -1);

	res = vulkan.device.createComputePipelines(nullptr, 1, &pipelineInfo, nullptr, &renderPass.voxelUpdater.pipeline);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateComputePipelines Error: ") + vk::to_string(res));
	}

	vulkan.device.destroyShaderModule(compShaderModule, nullptr);

	// Ray Tracer

	vk::PushConstantRange constantRange(vk::ShaderStageFlagBits::eCompute, 0, sizeof(renderPass.rayTracer.constants));
	
	layoutInfo = vk::PipelineLayoutCreateInfo(vk::PipelineLayoutCreateFlags(), 1, &renderPass.rayTracer.descriptor.layout, 1, &constantRange);

	res = vulkan.device.createPipelineLayout(&layoutInfo, nullptr, &renderPass.rayTracer.layout);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreatePipelineLayout Error: ") + vk::to_string(res));
	}

	compShaderCode = readFile("obj/shaders/rayTracer.comp.spv");
	compShaderModule = createShaderModule(compShaderCode);
	compStageInfo = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eCompute, compShaderModule, "main", nullptr);

	pipelineInfo = vk::ComputePipelineCreateInfo(vk::PipelineCreateFlags(), compStageInfo, renderPass.rayTracer.layout, nullptr, -1);

	res = vulkan.device.createComputePipelines(nullptr, 1, &pipelineInfo, nullptr, &renderPass.rayTracer.pipeline);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateComputePipelines Error: ") + vk::to_string(res));
	}

	vulkan.device.destroyShaderModule(compShaderModule, nullptr);

	// Toner Mapper

	layoutInfo = vk::PipelineLayoutCreateInfo(vk::PipelineLayoutCreateFlags(), 1, &renderPass.toneMapper.descriptor.layout, 0, nullptr);

	res = vulkan.device.createPipelineLayout(&layoutInfo, nullptr, &renderPass.toneMapper.layout);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreatePipelineLayout Error: ") + vk::to_string(res));
	}

	compShaderCode = readFile("obj/shaders/toneMapper.comp.spv");
	compShaderModule = createShaderModule(compShaderCode);
	compStageInfo = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eCompute, compShaderModule, "main", nullptr);

	pipelineInfo = vk::ComputePipelineCreateInfo(vk::PipelineCreateFlags(), compStageInfo, renderPass.toneMapper.layout, nullptr, -1);

	res = vulkan.device.createComputePipelines(nullptr, 1, &pipelineInfo, nullptr, &renderPass.toneMapper.pipeline);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateComputePipelines Error: ") + vk::to_string(res));
	}

	vulkan.device.destroyShaderModule(compShaderModule, nullptr);
}

static void createSemaphores() {
	vk::SemaphoreCreateInfo semaphoreInfo = vk::SemaphoreCreateInfo(vk::SemaphoreCreateFlags());

	vk::Result res;
	
	std::vector<vk::Semaphore*> semaphores = {
		&renderPass.render.first, &renderPass.render.second, &renderPass.finished, &presentationPass.available, &presentationPass.finished
	};

	for (vk::Semaphore* sem : semaphores) {
		res = vulkan.device.createSemaphore(&semaphoreInfo, nullptr, sem);

		if (res != vk::Result::eSuccess) {
			throw std::runtime_error(std::string("VK_CreateSemaphore Error: ") + vk::to_string(res));
		}
	}
}

static void createFences() {
	vk::FenceCreateInfo fenceInfo = vk::FenceCreateInfo(vk::FenceCreateFlags());

	vk::Result res = vulkan.device.createFence(&fenceInfo, nullptr, &feedbackPass.download);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateFence Error: ") + vk::to_string(res));
	}
}

static void createCommandPools() {
	// Feedback Pass

	vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlags(), feedbackPass.family);

	vk::Result res = vulkan.device.createCommandPool(&poolInfo, nullptr, &feedbackPass.pool);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateCommandPool Error: ") + vk::to_string(res));
	}

	// Render Pass

	poolInfo = vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, renderPass.family);

	res = vulkan.device.createCommandPool(&poolInfo, nullptr, &renderPass.pool);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateCommandPool Error: ") + vk::to_string(res));
	}

	// Presentation Pass

	poolInfo = vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlags(), presentationPass.family);

	res = vulkan.device.createCommandPool(&poolInfo, nullptr, &presentationPass.pool);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateCommandPool Error: ") + vk::to_string(res));
	}
}

static void createFeedbackCommandBuffer() {
	std::vector<vk::CommandBuffer*> buffers = {
		&feedbackPass.primary.first, &feedbackPass.primary.second
	};

	std::vector<vk::CommandBuffer> tmp(buffers.size());

	vk::CommandBufferAllocateInfo allocInfo(feedbackPass.pool, vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(buffers.size()));

	vk::Result res = vulkan.device.allocateCommandBuffers(&allocInfo, tmp.data());

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateCommandBuffers Error: ") + vk::to_string(res));
	}

	for (int i = 0; i < buffers.size(); i++) {
		*buffers[i] = tmp[i];
	}

	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlags(), nullptr);

	feedbackPass.primary.first.begin(&beginInfo);

	feedbackPass.primary.first.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), {}, {
		vk::BufferMemoryBarrier(vk::AccessFlags(), vk::AccessFlagBits::eTransferRead, renderPass.family, feedbackPass.family, feedbackPass.gpuStaging.buffer,
		0, feedbackPass.size)
		}, {});

	feedbackPass.primary.first.copyBuffer(feedbackPass.gpuStaging.buffer, feedbackPass.cpuStaging.buffer, { vk::BufferCopy(0, 0, feedbackPass.size) });

	feedbackPass.primary.first.end();

	feedbackPass.primary.second.begin(&beginInfo);

	feedbackPass.primary.second.copyBuffer(feedbackPass.cpuStaging.buffer, feedbackPass.gpuStaging.buffer, { vk::BufferCopy(0, 0, feedbackPass.size) });

	feedbackPass.primary.second.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe, vk::DependencyFlags(), {}, {
		vk::BufferMemoryBarrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlags(), feedbackPass.family, renderPass.family, feedbackPass.gpuStaging.buffer,
			0, feedbackPass.size)
	}, {});

	feedbackPass.primary.second.end();
}

static void createRenderCommandBuffers() {
	std::vector<vk::CommandBuffer*> buffers = {
		&renderPass.primary.first, &renderPass.primary.second
	};

	std::vector<vk::CommandBuffer> tmp(buffers.size());

	vk::CommandBufferAllocateInfo allocInfo(renderPass.pool, vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(buffers.size()));

	vk::Result res = vulkan.device.allocateCommandBuffers(&allocInfo, tmp.data());

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateCommandBuffers Error: ") + vk::to_string(res));
	}

	for (int i = 0; i < buffers.size(); i++) {
		*buffers[i] = tmp[i];
	}

	buffers = {
		&renderPass.voxelUpdater.secondary, &renderPass.rayTracer.secondary, &renderPass.toneMapper.secondary
	};

	tmp.resize(buffers.size());

	allocInfo = vk::CommandBufferAllocateInfo(renderPass.pool, vk::CommandBufferLevel::eSecondary, static_cast<uint32_t>(buffers.size()));

	res = vulkan.device.allocateCommandBuffers(&allocInfo, tmp.data());

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateCommandBuffers Error: ") + vk::to_string(res));
	}

	for (int i = 0; i < buffers.size(); i++) {
		*buffers[i] = tmp[i];
	}

	vk::CommandBufferInheritanceInfo inheritanceInfo(nullptr, -1, nullptr, false, vk::QueryControlFlags(), vk::QueryPipelineStatisticFlags());
	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlags(), &inheritanceInfo);

	renderPass.toneMapper.secondary.begin(&beginInfo);

	renderPass.toneMapper.secondary.bindPipeline(vk::PipelineBindPoint::eCompute, renderPass.toneMapper.pipeline);
	renderPass.toneMapper.secondary.bindDescriptorSets(vk::PipelineBindPoint::eCompute, renderPass.toneMapper.layout, 0, 1, &renderPass.toneMapper.descriptor.set, 0, nullptr);
	renderPass.toneMapper.secondary.dispatch(1, 1, 1);

	renderPass.toneMapper.secondary.end();

	beginInfo = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlags(), nullptr);

	renderPass.primary.second.begin(&beginInfo);
	renderPass.primary.second.executeCommands({ renderPass.toneMapper.secondary });
	renderPass.primary.second.end();
}

static void recordRenderCommandBuffers(bool voxelUpdater, bool rayTracer) {
	if (!voxelUpdater && !rayTracer) {
		return;
	}

	vk::CommandBufferInheritanceInfo inheritanceInfo(nullptr, -1, nullptr, false, vk::QueryControlFlags(), vk::QueryPipelineStatisticFlags());
	vk::CommandBufferBeginInfo beginInfo(vk::CommandBufferUsageFlags(), &inheritanceInfo);

	if (voxelUpdater) {
		renderPass.voxelUpdater.secondary.begin(&beginInfo);

		renderPass.voxelUpdater.secondary.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags(), {}, {
			vk::BufferMemoryBarrier(vk::AccessFlags(), vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite, feedbackPass.family, renderPass.family,
				feedbackPass.gpuStaging.buffer, 0, feedbackPass.size)
		}, {});

		renderPass.voxelUpdater.secondary.bindPipeline(vk::PipelineBindPoint::eCompute, renderPass.voxelUpdater.pipeline);
		renderPass.voxelUpdater.secondary.bindDescriptorSets(vk::PipelineBindPoint::eCompute, renderPass.voxelUpdater.layout, 0, 1, &renderPass.voxelUpdater.descriptor.set, 0, nullptr);
		renderPass.voxelUpdater.secondary.dispatch(1, 1, 1);

		renderPass.voxelUpdater.secondary.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), {}, {
			vk::BufferMemoryBarrier(vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eTransferWrite, renderPass.family, renderPass.family,
				feedbackPass.gpuStaging.buffer, 0, feedbackPass.size)
		}, {});

		renderPass.voxelUpdater.secondary.fillBuffer(feedbackPass.gpuStaging.buffer, 0, feedbackPass.size, 0x00);

		renderPass.voxelUpdater.secondary.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags(), {}, {
			vk::BufferMemoryBarrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite, renderPass.family, renderPass.family,
				feedbackPass.gpuStaging.buffer, 0, feedbackPass.size)
		}, {});

		renderPass.voxelUpdater.secondary.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags(), {}, {}, {
			vk::ImageMemoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
				renderPass.family, renderPass.family, voxelBuffer.structure.image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))
		});

		renderPass.voxelUpdater.secondary.end();
	}

	if (rayTracer) {
		renderPass.rayTracer.secondary.begin(&beginInfo);

		renderPass.rayTracer.secondary.bindPipeline(vk::PipelineBindPoint::eCompute, renderPass.rayTracer.pipeline);
		renderPass.rayTracer.secondary.bindDescriptorSets(vk::PipelineBindPoint::eCompute, renderPass.rayTracer.layout, 0, 1, &renderPass.rayTracer.descriptor.set, 0, nullptr);
		renderPass.rayTracer.secondary.pushConstants(renderPass.rayTracer.layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(renderPass.rayTracer.constants), &renderPass.rayTracer.constants);
		renderPass.rayTracer.secondary.dispatch(1, 1, 1);

		renderPass.rayTracer.secondary.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eBottomOfPipe, vk::DependencyFlags(), {}, {
			vk::BufferMemoryBarrier(vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite, vk::AccessFlags(), renderPass.family, feedbackPass.family,
				feedbackPass.gpuStaging.buffer, 0, feedbackPass.size)
		}, {});

		renderPass.rayTracer.secondary.end();
	}

	renderPass.primary.first.begin(&beginInfo);
	renderPass.primary.first.executeCommands({ renderPass.voxelUpdater.secondary, renderPass.rayTracer.secondary });
	renderPass.primary.first.end();
}

static void createPresentationCommandBuffers() {
	presentationPass.primary.resize(presentationPass.swapChain.images.size());

	vk::CommandBufferAllocateInfo allocInfo(presentationPass.pool, vk::CommandBufferLevel::ePrimary, static_cast<uint32_t>(presentationPass.primary.size()));

	vk::Result res = vulkan.device.allocateCommandBuffers(&allocInfo, presentationPass.primary.data());

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateCommandBuffers Error: ") + vk::to_string(res));
	}

	vk::CommandBufferBeginInfo beginInfo;

	for (size_t i = 0; i < presentationPass.primary.size(); i++) {
		beginInfo = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlags(), nullptr);
		presentationPass.primary[i].begin(&beginInfo);

		presentationPass.primary[i].pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), {}, {}, {
			vk::ImageMemoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eTransferRead, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferSrcOptimal,
				renderPass.family, renderPass.family, renderPass.ldrImage.image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))
		});

		presentationPass.primary[i].pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(), {}, {}, {
			vk::ImageMemoryBarrier(vk::AccessFlagBits::eColorAttachmentRead, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
				presentationPass.family, presentationPass.family, presentationPass.swapChain.images[i], vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))
		});

		presentationPass.primary[i].blitImage(renderPass.ldrImage.image, vk::ImageLayout::eTransferSrcOptimal, presentationPass.swapChain.images[i], vk::ImageLayout::eTransferDstOptimal,
			{ vk::ImageBlit(vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), { vk::Offset3D(0, 0, 0), vk::Offset3D(renderPass.ldrImage.extent.width,
				renderPass.ldrImage.extent.height, 1) }, vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1), { vk::Offset3D(0, 0, 0), vk::Offset3D(
					presentationPass.swapChain.extent.width, presentationPass.swapChain.extent.height, 1) }) }, vk::Filter::eNearest);

		presentationPass.primary[i].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlags(), {}, {}, {
			vk::ImageMemoryBarrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eColorAttachmentRead, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR,
				presentationPass.family, presentationPass.family, presentationPass.swapChain.images[i], vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))
		});

		presentationPass.primary[i].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eComputeShader, vk::DependencyFlags(), {}, {}, {
			vk::ImageMemoryBarrier(vk::AccessFlagBits::eTransferRead, vk::AccessFlagBits::eShaderWrite, vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eGeneral,
				presentationPass.family, renderPass.family, renderPass.ldrImage.image, vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1))
		});

		presentationPass.primary[i].end();
	}
}

static void recreateSwapChain() {
	vulkan.device.waitIdle();

	vk::SurfaceCapabilitiesKHR surfaceCapabilities = vulkan.physical.getSurfaceCapabilitiesKHR(vulkan.surface);

	if (surfaceCapabilities.currentExtent.width == 0 || surfaceCapabilities.currentExtent.height == 0) {
		return;
	}

	cleanupSwapChain();

	createSwapChain();

	createPresentationCommandBuffers();
}

static void cleanupSwapChain() {
	vulkan.device.freeCommandBuffers(presentationPass.pool, presentationPass.primary);

	vulkan.device.destroySwapchainKHR(presentationPass.swapChain.queue, nullptr);
}