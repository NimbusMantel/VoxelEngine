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

const uint16_t WIDTH = 640;
const uint16_t HEIGHT = 360;
const uint8_t  FOV = 70;

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

vk::DescriptorPool descriptorPool;
vk::DescriptorSetLayout descriptorLayout;
vk::DescriptorSet descriptorSet;

vk::PipelineLayout pipelineLayout;
vk::Pipeline computePipeline;

vk::CommandPool renderPool;
vk::CommandPool transferPool;
std::vector<vk::CommandBuffer> commandBuffers;

vk::Semaphore imageAvailable;
vk::Semaphore renderFinished;

struct OffscreenPass {
	vk::Format format = vk::Format::eR16G16B16A16Sfloat;
	vk::Extent2D extent = vk::Extent2D(WIDTH, HEIGHT);
	vk::ClearValue clearColor = vk::ClearColorValue(std::array<float, 4>( { 0.0f, 0.0f, 0.0f, 0.0f }));

	vk::Image image;
	vk::DeviceMemory memory;
	vk::ImageView view;

	vk::CommandBuffer commandBuffer;

	vk::Semaphore semaphore;

	struct Constants {
		glm::mat3 rot = glm::mat3(1.0f);
		glm::vec3 pos = glm::vec3(0.0f, 1.0f, 0.0f);
		float     fov = tanf(M_PI * FOV / 360.0f);
	} constants;
} offscreenPass;

struct RayQueue {
	vk::DeviceSize size = sizeof(uint32_t);

	vk::Buffer buffer;
	vk::DeviceMemory memory;
} rayQueue;

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

static void updateState();
static void drawFrame();

static void createInstance();
static void createDebugCallback();
static void createSurface();
static void pickPhysicalDevice();
static void createLogicalDevice();

static void createSwapChain();
static void createImage();
static void createImageView();

static void createBuffers();

static void createDescriptorPool();
static void createDescriptorSet();
static void createComputePipeline();

static void createCommandPools();
static void createRenderCommandBuffer();
static void recordRenderCommandBuffer();
static void createTransferCommandBuffers();

static void createSemaphores();

static void recreateSwapChain();
static void cleanupSwapChain();

int SDL_main(int argc, char* argv[]) {
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
	createImage();
	createImageView();

	createBuffers();

	createDescriptorPool();
	createDescriptorSet();
	createComputePipeline();

	createCommandPools();
	createRenderCommandBuffer();
	recordRenderCommandBuffer();
	createTransferCommandBuffers();

	createSemaphores();
}

static void update() {
	SDL_Event event;

	bool quit = false;

	while (!quit) {
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
					uint32_t width, height;

					SDL_GetWindowSize(window, (int*)&width, (int*)&height);

					swapChainExtent = vk::Extent2D(std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, width)),
						std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, height)));
				}
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_ESCAPE) {
					quit = true;
				}
				else if (event.key.keysym.sym == SDLK_f) {
					isFullscreen = !isFullscreen;

					SDL_SetWindowFullscreen(window, isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);

					recreateSwapChain();
				}
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

	device.freeCommandBuffers(renderPool, { offscreenPass.commandBuffer });
	device.destroyCommandPool(renderPool, nullptr);
	device.freeCommandBuffers(transferPool, commandBuffers);
	device.destroyCommandPool(transferPool, nullptr);

	device.destroyPipeline(computePipeline, nullptr);
	device.destroyPipelineLayout(pipelineLayout, nullptr);

	device.destroyDescriptorSetLayout(descriptorLayout, nullptr);
	device.destroyDescriptorPool(descriptorPool, nullptr);

	device.destroyBuffer(rayQueue.buffer, nullptr);
	device.freeMemory(rayQueue.memory, nullptr);

	device.destroyImageView(offscreenPass.view, nullptr);
	device.destroyImage(offscreenPass.image, nullptr);
	device.freeMemory(offscreenPass.memory, nullptr);

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

static void updateState() {
	// Update position and rotation

	//recordRenderCommandBuffer();
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

	vk::SubmitInfo submitInfo(1, &imageAvailable, waitStages, 1, &offscreenPass.commandBuffer, 1, &offscreenPass.semaphore);

	res = computeQueue.submit(1, &submitInfo, nullptr);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_QueueSubmit Error: ") + vk::to_string(res));
	}

	waitStages[0] = vk::PipelineStageFlagBits::eComputeShader;

	submitInfo = vk::SubmitInfo(1, &offscreenPass.semaphore, waitStages, 1, &commandBuffers[imageIndex], 1, &renderFinished);

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
	deviceFeatures.geometryShader = true;

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

static void createImage() {
	vk::ImageCreateInfo offscreenInfo = vk::ImageCreateInfo(vk::ImageCreateFlags(), vk::ImageType::e2D, offscreenPass.format, vk::Extent3D(WIDTH, HEIGHT, 1), 1, 1, vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal,vk::ImageUsageFlags(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc), vk::SharingMode::eExclusive,
		1, &queueIndices.computeFamily, vk::ImageLayout::eUndefined);

	vk::Result res = device.createImage(&offscreenInfo, nullptr, &offscreenPass.image);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateImage Error: ") + vk::to_string(res));
	}

	vk::MemoryRequirements memReqs = device.getImageMemoryRequirements(offscreenPass.image);

	vk::MemoryAllocateInfo allocInfo = vk::MemoryAllocateInfo(memReqs.size, getMemoryTypeIndex(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

	res = device.allocateMemory(&allocInfo, nullptr, &offscreenPass.memory);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateMemory Error: ") + vk::to_string(res));
	}

	device.bindImageMemory(offscreenPass.image, offscreenPass.memory, 0);
}

static void createImageView() {
	vk::ImageViewCreateInfo viewInfo(vk::ImageViewCreateFlags(), offscreenPass.image, vk::ImageViewType::e2D, offscreenPass.format, vk::ComponentMapping(vk::ComponentSwizzle::eIdentity,
		vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

	vk::Result res = device.createImageView(&viewInfo, nullptr, &offscreenPass.view);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateImageView Error: ") + vk::to_string(res));
	}
}

static void createBuffers() {
	vk::BufferCreateInfo bufferInfo = vk::BufferCreateInfo(vk::BufferCreateFlags(), rayQueue.size, vk::BufferUsageFlagBits::eStorageBuffer, vk::SharingMode::eExclusive, 1, &queueIndices.computeFamily);

	vk::Result res = device.createBuffer(&bufferInfo, nullptr, &rayQueue.buffer);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateBuffer Error: ") + vk::to_string(res));
	}

	vk::MemoryRequirements memReqs = device.getBufferMemoryRequirements(rayQueue.buffer);

	vk::MemoryAllocateInfo allocInfo = vk::MemoryAllocateInfo(memReqs.size, getMemoryTypeIndex(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));

	res = device.allocateMemory(&allocInfo, nullptr, &rayQueue.memory);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateMemory Error: ") + vk::to_string(res));
	}

	device.bindBufferMemory(rayQueue.buffer, rayQueue.memory, 0);
}

static void createDescriptorPool() {
	std::vector<vk::DescriptorPoolSize> poolSizes = {
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1),
		vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1)
	};

	vk::DescriptorPoolCreateInfo poolInfo(vk::DescriptorPoolCreateFlags(), 1, static_cast<uint32_t>(poolSizes.size()), poolSizes.data());

	vk::Result res = device.createDescriptorPool(&poolInfo, nullptr, &descriptorPool);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDescriptorPool Error: ") + vk::to_string(res));
	}
}

static void createDescriptorSet() {
	std::vector<vk::DescriptorSetLayoutBinding> bindings = {
		vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageImage, 1, vk::ShaderStageFlagBits::eCompute, nullptr),
		vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute, nullptr)
	};

	vk::DescriptorSetLayoutCreateInfo layoutInfo(vk::DescriptorSetLayoutCreateFlags(), static_cast<uint32_t>(bindings.size()), bindings.data());

	vk::Result res = device.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorLayout);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDescriptorSetLayout Error: ") + vk::to_string(res));
	}

	vk::DescriptorSetAllocateInfo allocInfo(descriptorPool, 1, &descriptorLayout);

	res = device.allocateDescriptorSets(&allocInfo, &descriptorSet);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateDescriptorSets Error: ") + vk::to_string(res));
	}

	vk::DescriptorImageInfo imageInfo(nullptr, offscreenPass.view, vk::ImageLayout::eGeneral);
	vk::DescriptorBufferInfo bufferInfo = vk::DescriptorBufferInfo(rayQueue.buffer, 0, rayQueue.size);

	std::vector<vk::WriteDescriptorSet> descriptorWrites = {
		vk::WriteDescriptorSet(descriptorSet, 0, 0, 1, vk::DescriptorType::eStorageImage, &imageInfo, nullptr, nullptr),
		vk::WriteDescriptorSet(descriptorSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &bufferInfo, nullptr)
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

static void createComputePipeline() {
	vk::PushConstantRange constantRange(vk::ShaderStageFlagBits::eCompute, 0, sizeof(offscreenPass.constants));
	
	vk::PipelineLayoutCreateInfo layoutInfo(vk::PipelineLayoutCreateFlags(), 1, &descriptorLayout, 1, &constantRange);

	vk::Result res = device.createPipelineLayout(&layoutInfo, nullptr, &pipelineLayout);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreatePipelineLayout Error: ") + vk::to_string(res));
	}

	std::vector<char> compShaderCode = readFile("obj/shaders/test.comp.spv");
	vk::ShaderModule compShaderModule = createShaderModule(compShaderCode);
	vk::PipelineShaderStageCreateInfo compStageInfo = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eCompute, compShaderModule, "main", nullptr);

	vk::ComputePipelineCreateInfo pipelineInfo(vk::PipelineCreateFlags(), compStageInfo, pipelineLayout, nullptr, -1);

	res = device.createComputePipelines(nullptr, 1, &pipelineInfo, nullptr, &computePipeline);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateComputePipelines Error: ") + vk::to_string(res));
	}

	device.destroyShaderModule(compShaderModule, nullptr);
}

static void createCommandPools() {
	vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer), queueIndices.computeFamily);

	vk::Result res = device.createCommandPool(&poolInfo, nullptr, &renderPool);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateCommandPool Error: ") + vk::to_string(res));
	}

	poolInfo = vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits(0)), queueIndices.transferFamily);

	res = device.createCommandPool(&poolInfo, nullptr, &transferPool);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateCommandPool Error: ") + vk::to_string(res));
	}
}

static void createRenderCommandBuffer() {
	vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo(renderPool, vk::CommandBufferLevel::ePrimary, 1);

	vk::Result res = device.allocateCommandBuffers(&allocInfo, &offscreenPass.commandBuffer);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateCommandBuffers Error: ") + vk::to_string(res));
	}
}

static void recordRenderCommandBuffer() {
	vk::CommandBufferBeginInfo commandBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse, nullptr);

	offscreenPass.commandBuffer.begin(&commandBeginInfo);

	offscreenPass.commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
	offscreenPass.commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
	offscreenPass.commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(offscreenPass.constants), &offscreenPass.constants);
	offscreenPass.commandBuffer.dispatch(1, 1, 1);

	vk::Result res = vk::Result(vkEndCommandBuffer(offscreenPass.commandBuffer)); // use C API to get result value

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_EndCommandBuffer Error: ") + vk::to_string(res));
	}
}

static void createTransferCommandBuffers() {
	commandBuffers.resize(swapChainImages.size());

	vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo(transferPool, vk::CommandBufferLevel::ePrimary, (uint32_t)commandBuffers.size());

	vk::Result res = device.allocateCommandBuffers(&allocInfo, commandBuffers.data());

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateCommandBuffers Error: ") + vk::to_string(res));
	}

	vk::CommandBufferBeginInfo commandBeginInfo;

	vk::ImageSubresourceLayers offsetLayer(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
	std::array<vk::Offset3D, 2> offsetExtent = { vk::Offset3D(0, 0, 0), vk::Offset3D(offscreenPass.extent.width, offscreenPass.extent.height, 1) };

	for (size_t i = 0; i < commandBuffers.size(); i++) {
		commandBeginInfo = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse, nullptr);
		commandBuffers[i].begin(&commandBeginInfo);

		commandBuffers[i].pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlagBits::eByRegion, {}, {},
			{ vk::ImageMemoryBarrier(vk::AccessFlagBits::eColorAttachmentRead, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
				queueIndices.presentFamily, queueIndices.transferFamily, swapChainImages[i], vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)) });

		vk::ImageSubresourceLayers imageLayer = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
		std::array<vk::Offset3D, 2> imageExtent = { vk::Offset3D(0, 0, 0), vk::Offset3D(swapChainExtent.width, swapChainExtent.height, 1) };

		commandBuffers[i].blitImage(offscreenPass.image, vk::ImageLayout::eTransferSrcOptimal, swapChainImages[i], vk::ImageLayout::eTransferDstOptimal,
			std::array<vk::ImageBlit, 1>({ vk::ImageBlit(offsetLayer, offsetExtent, imageLayer, imageExtent) }), vk::Filter::eNearest);

		commandBuffers[i].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eAllCommands, vk::DependencyFlagBits::eByRegion, {}, {},
			{ vk::ImageMemoryBarrier(vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eColorAttachmentRead, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR,
				queueIndices.transferFamily, queueIndices.presentFamily, swapChainImages[i], vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)) });

		res = vk::Result(vkEndCommandBuffer(commandBuffers[i])); // use C API to get result value

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
	device.freeCommandBuffers(transferPool, commandBuffers);

	device.destroySwapchainKHR(swapChain, nullptr);
}