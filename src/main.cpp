#include <SDL2/SDL.h>
#include <vulkan/vulkan.hpp>

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

SDL_Window* window;

vk::Instance instance;
vk::DebugReportCallbackEXT callback;

vk::SurfaceKHR surface;

vk::Device device;

vk::Queue graphicsQueue;
vk::Queue computeQueue;
vk::Queue presentationQueue;

vk::SwapchainKHR swapChain;

std::vector<vk::Image> swapChainImages;
std::vector<vk::ImageView> swapChainImageViews;

vk::Format swapChainFormat;
vk::Extent2D swapChainExtent;

vk::Viewport viewport;

vk::RenderPass renderPass;
vk::PipelineLayout pipelineLayout;

vk::Pipeline graphicsPipeline;

std::vector<vk::Framebuffer> swapChainFramebuffers;

vk::CommandPool commandPool;
std::vector<vk::CommandBuffer> commandBuffers;

vk::Semaphore imageAvailable;
vk::Semaphore renderFinished;

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

static vk::ShaderModule createShaderModule(const std::vector<char>& code) {
	vk::ShaderModuleCreateInfo moduleInfo = vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(), code.size(), reinterpret_cast<const uint32_t*>(code.data()));
	
	vk::ShaderModule shaderModule;

	vk::Result res = device.createShaderModule(&moduleInfo, nullptr, &shaderModule);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateShaderModule Error: ") + vk::to_string(res));
	}

	return shaderModule;
}

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
	const std::vector<const char*> layers = {};
#else
	const std::vector<const char*> layers = {
		"VK_LAYER_LUNARG_standard_validation"
	};
#endif

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

	vk::InstanceCreateInfo instInfo(vk::InstanceCreateFlags(), &appInfo, static_cast<uint32_t>(layers.size()), layers.data(), static_cast<uint32_t>(instExt.size()), instExt.data());

	try {
		instance = vk::createInstance(instInfo);
	}
	catch (std::exception e) {
		throw std::runtime_error(std::string("VK_CreateInstance Error: ") + e.what());
	}

	vk::Result res;

#ifndef NDEBUG
	vk::DebugReportCallbackCreateInfoEXT callInfo(vk::DebugReportFlagBitsEXT::eError | vk::DebugReportFlagBitsEXT::eWarning, debugCallback);

	res = vk::Result(((PFN_vkCreateDebugReportCallbackEXT)instance.getProcAddr("vkCreateDebugReportCallbackEXT"))(instance,
		reinterpret_cast<const VkDebugReportCallbackCreateInfoEXT*>(&callInfo), nullptr, reinterpret_cast<VkDebugReportCallbackEXT*>(&callback)));

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDebugReportCallbackEXT Error: ") + vk::to_string(res));
	}
#endif

	if (SDL_Vulkan_CreateSurface(window, instance, reinterpret_cast<VkSurfaceKHR*>(&surface)) != SDL_TRUE) {
		throw std::runtime_error(std::string("SDL_Vulkan_CreateSurface Error: ") + SDL_GetError());
	}

	std::vector<vk::PhysicalDevice> physDevs = instance.enumeratePhysicalDevices();

	if (physDevs.size() == 0) {
		throw std::runtime_error(std::string("VK_EnumeratePhysicalDevices Error: ") + "ErrorVulkanGPUNotPresent");
	}

	const std::vector<const char*> devExt = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	uint32_t maxGPUScore = 0;
	uint32_t gpuScore;

	vk::PhysicalDeviceProperties devProps;
	vk::PhysicalDeviceFeatures devFeats;

	vk::PhysicalDevice selDev;

	std::vector<vk::QueueFamilyProperties> queueFamilies;

	uint32_t graphicsQueueIndex, computeQueueIndex, presentationQueueIndex;

	uint32_t graQueIdx, comQueIdx, prsQueIdx;
	uint32_t tmp;

	std::vector<vk::ExtensionProperties> avaExt;
	std::set<std::string> reqExt;

	vk::SurfaceCapabilitiesKHR surfaceCapabilities;
	std::vector<vk::SurfaceFormatKHR> surfaceFormats;
	std::vector<vk::PresentModeKHR> presentModes;

	vk::SurfaceCapabilitiesKHR surCap;
	std::vector<vk::SurfaceFormatKHR> surFor;
	std::vector<vk::PresentModeKHR> prsMod;

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

		graQueIdx = comQueIdx = -1;

		tmp = 0;

		for (const vk::QueueFamilyProperties queFamily : queueFamilies) {
			if (queFamily.queueCount > 0) {
				if (queFamily.queueFlags & vk::QueueFlags(vk::QueueFlagBits::eGraphics)) {
					graQueIdx = tmp;
				}
				
				if (queFamily.queueFlags & vk::QueueFlags(vk::QueueFlagBits::eCompute)) {
					comQueIdx = tmp;

					gpuScore += (comQueIdx == graQueIdx) << 5;
				}

				if (dev.getSurfaceSupportKHR(tmp, surface)) {
					prsQueIdx = tmp;

					gpuScore += ((comQueIdx == graQueIdx) || (comQueIdx == comQueIdx)) << 5;
				}
			}

			tmp++;
		}

		if (graQueIdx == -1 || comQueIdx == -1 || prsQueIdx == -1) {
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

		surCap = dev.getSurfaceCapabilitiesKHR(surface);
		surFor = dev.getSurfaceFormatsKHR(surface);
		prsMod = dev.getSurfacePresentModesKHR(surface);

		if (surFor.empty() || prsMod.empty()) {
			continue;
		}

		if (gpuScore > maxGPUScore) {
			maxGPUScore = gpuScore;

			selDev = dev;

			graphicsQueueIndex = graQueIdx;
			computeQueueIndex = comQueIdx;
			presentationQueueIndex = prsQueIdx;

			surfaceCapabilities = surCap;
			surfaceFormats = surFor;
			presentModes = prsMod;
		}
	}

	if (maxGPUScore <= 0) {
		throw std::runtime_error(std::string("VK_EnumeratePhysicalDevices Error: ") + "ErrorSuitableGPUNotPresent");
	}

	std::vector<vk::DeviceQueueCreateInfo> queueInfos;
	std::set<uint32_t> uniqueQueueFamilies = { graphicsQueueIndex, computeQueueIndex, presentationQueueIndex };

	float queuePriority = 1.0f;

	for (const uint32_t queFam : uniqueQueueFamilies) {
		queueInfos.push_back(vk::DeviceQueueCreateInfo(vk::DeviceQueueCreateFlags(), queFam, 1, &queuePriority));
	}

	vk::PhysicalDeviceFeatures deviceFeatures;
	deviceFeatures.geometryShader = true;
	
	vk::DeviceCreateInfo deviceInfo(vk::DeviceCreateFlags(), static_cast<uint32_t>(queueInfos.size()), queueInfos.data(),
		static_cast<uint32_t>(layers.size()), layers.data(), static_cast<uint32_t>(devExt.size()), devExt.data(), &deviceFeatures);

	res = selDev.createDevice(&deviceInfo, nullptr, &device);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateDevice Error: ") + vk::to_string(res));
	}

	graphicsQueue = device.getQueue(graphicsQueueIndex, 0);
	computeQueue = device.getQueue(computeQueueIndex, 0);
	presentationQueue = device.getQueue(presentationQueueIndex, 0);

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
	
	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		swapChainExtent = surfaceCapabilities.currentExtent;
	}
	else {
		swapChainExtent = vk::Extent2D(std::max(static_cast<uint16_t>(surfaceCapabilities.minImageExtent.width), std::min(static_cast<uint16_t>(surfaceCapabilities.maxImageExtent.width), WIDTH)),
			std::max(static_cast<uint16_t>(surfaceCapabilities.minImageExtent.height), std::min(static_cast<uint16_t>(surfaceCapabilities.maxImageExtent.height), HEIGHT)));
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
	swapInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

	std::vector<uint32_t> queueIndices(uniqueQueueFamilies.size());

	for (const uint32_t queIdx : uniqueQueueFamilies) {
		queueIndices.push_back(queIdx);
	}
	
	if (uniqueQueueFamilies.size() > 1) {
		swapInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		swapInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueIndices.size());
		swapInfo.pQueueFamilyIndices = queueIndices.data();
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

	res = device.createSwapchainKHR(&swapInfo, nullptr, &swapChain);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateSwapchainKHR Error: ") + vk::to_string(res));
	}

	swapChainImages = device.getSwapchainImagesKHR(swapChain);
	swapChainFormat = surfaceFormat.format;

	swapChainImageViews.resize(swapChainImages.size());

	vk::ImageViewCreateInfo viewInfo;

	tmp = 0;

	for (const vk::Image img : swapChainImages) {
		viewInfo = vk::ImageViewCreateInfo(vk::ImageViewCreateFlags(), img, vk::ImageViewType::e2D, swapChainFormat, vk::ComponentMapping(vk::ComponentSwizzle::eIdentity,
			vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity, vk::ComponentSwizzle::eIdentity), vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

		res = device.createImageView(&viewInfo, nullptr, &swapChainImageViews[tmp]);

		if (res != vk::Result::eSuccess) {
			throw std::runtime_error(std::string("VK_CreateImageView Error: ") + vk::to_string(res));
		}

		tmp++;
	}

	vk::AttachmentDescription colorAttachment(vk::AttachmentDescriptionFlags(), swapChainFormat, vk::SampleCountFlagBits::e1, vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare, vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);
	vk::AttachmentReference colorReference(0, vk::ImageLayout::eColorAttachmentOptimal);

	vk::SubpassDescription subpass(vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorReference, nullptr, nullptr, 0, nullptr);

	vk::SubpassDependency dependency(VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlags(vk::PipelineStageFlagBits::eColorAttachmentOutput), vk::PipelineStageFlags(vk::PipelineStageFlagBits::
		eColorAttachmentOutput), vk::AccessFlags(vk::AccessFlagBits(0)), vk::AccessFlags(vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite), vk::DependencyFlags());

	vk::RenderPassCreateInfo renderInfo = vk::RenderPassCreateInfo(vk::RenderPassCreateFlags(), 1, &colorAttachment, 1, &subpass, 1, &dependency);

	res = device.createRenderPass(&renderInfo, nullptr, &renderPass);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateRenderPass Error: ") + vk::to_string(res));
	}

	std::vector<char> vertShaderCode = readFile("obj/shaders/test.vert.spv");
	std::vector<char> fragShaderCode = readFile("obj/shaders/test.frag.spv");

	vk::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
	vk::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	vk::PipelineShaderStageCreateInfo vertStageInfo = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex, vertShaderModule, "main", nullptr);
	vk::PipelineShaderStageCreateInfo fragStageInfo = vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main", nullptr);

	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertStageInfo, fragStageInfo };

	vk::PipelineVertexInputStateCreateInfo vertexInfo(vk::PipelineVertexInputStateCreateFlags(), 0, nullptr, 0, nullptr);
	vk::PipelineInputAssemblyStateCreateInfo inputInfo(vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList, false);

	viewport = vk::Viewport(0.0f, 0.0f, (float)swapChainExtent.width, (float)swapChainExtent.height, 0.0f, 1.0f);
	vk::Rect2D scissor = vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent);
	vk::PipelineViewportStateCreateInfo viewportInfo(vk::PipelineViewportStateCreateFlags(), 1, &viewport, 1, &scissor);

	vk::PipelineRasterizationStateCreateInfo rasterizerInfo(vk::PipelineRasterizationStateCreateFlags(), false, false, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise,
		false, 0.0f, 0.0f, 0.0f, 1.0f);

	vk::PipelineMultisampleStateCreateInfo multisamplerInfo(vk::PipelineMultisampleStateCreateFlags(), vk::SampleCountFlagBits::e1, false, 1.0f, nullptr, false, false);

	vk::PipelineColorBlendAttachmentState colorBlend(false, vk::BlendFactor::eOne, vk::BlendFactor::eZero, vk::BlendOp::eAdd, vk::BlendFactor::eOne, vk::BlendFactor::eZero,
		vk::BlendOp::eAdd, vk::ColorComponentFlags());
	vk::PipelineColorBlendStateCreateInfo colorInfo = vk::PipelineColorBlendStateCreateInfo(vk::PipelineColorBlendStateCreateFlags(), false, vk::LogicOp::eCopy, 1, &colorBlend, { 0.0f, 0.0f, 0.0f, 0.0f });

	/*vk::DynamicState dynamicStates[] = {
		vk::DynamicState::eViewport,
		vk::DynamicState::eLineWidth
	};

	vk::PipelineDynamicStateCreateInfo dynamicInfo = (vk::PipelineDynamicStateCreateFlags(), 2, dynamicStates);*/

	vk::PipelineLayoutCreateInfo layoutInfo(vk::PipelineLayoutCreateFlags(), 0, nullptr, 0, nullptr);

	res = device.createPipelineLayout(&layoutInfo, nullptr, &pipelineLayout);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreatePipelineLayout Error: ") + vk::to_string(res));
	}

	vk::GraphicsPipelineCreateInfo pipelineInfo = vk::GraphicsPipelineCreateInfo(vk::PipelineCreateFlags(), 2, shaderStages, &vertexInfo, &inputInfo, nullptr, &viewportInfo, &rasterizerInfo,
		&multisamplerInfo, nullptr, &colorInfo, nullptr, pipelineLayout, renderPass, 0, nullptr, -1);

	res = device.createGraphicsPipelines(nullptr, 1, &pipelineInfo, nullptr, &graphicsPipeline);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateGraphicsPipelines Error: ") + vk::to_string(res));
	}

	device.destroyShaderModule(vertShaderModule, nullptr);
	device.destroyShaderModule(fragShaderModule, nullptr);

	swapChainFramebuffers.resize(swapChainImageViews.size());

	vk::FramebufferCreateInfo framebufferInfo;

	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		framebufferInfo = vk::FramebufferCreateInfo(vk::FramebufferCreateFlags(), renderPass, 1, &swapChainImageViews[i], swapChainExtent.width, swapChainExtent.height, 1);
		
		res = device.createFramebuffer(&framebufferInfo, nullptr, &swapChainFramebuffers[i]);

		if (res != vk::Result::eSuccess) {
			throw std::runtime_error(std::string("VK_CreateFramebuffer Error: ") + vk::to_string(res));
		}
	}

	vk::CommandPoolCreateInfo poolInfo(vk::CommandPoolCreateFlags(), graphicsQueueIndex);

	res = device.createCommandPool(&poolInfo, nullptr, &commandPool);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateCommandPool Error: ") + vk::to_string(res));
	}

	commandBuffers.resize(swapChainFramebuffers.size());

	vk::CommandBufferAllocateInfo allocInfo = vk::CommandBufferAllocateInfo(commandPool, vk::CommandBufferLevel::ePrimary, (uint32_t)commandBuffers.size());

	res = device.allocateCommandBuffers(&allocInfo, commandBuffers.data());

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_AllocateCommandBuffers Error: ") + vk::to_string(res));
	}

	vk::CommandBufferBeginInfo commandBeginInfo;
	vk::RenderPassBeginInfo renderBeginInfo;

	vk::ClearValue clearColor(vk::ClearColorValue(std::array<float, 4>({ 1.0f, 0.0f, 0.0f, 1.0f })));

	for (size_t i = 0; i < commandBuffers.size(); i++) {
		commandBeginInfo = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse, nullptr);
		commandBuffers[i].begin(&commandBeginInfo);

		renderBeginInfo = vk::RenderPassBeginInfo(renderPass, swapChainFramebuffers[i], scissor, 1, &clearColor);
		commandBuffers[i].beginRenderPass(&renderBeginInfo, vk::SubpassContents::eInline);

		commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
		commandBuffers[i].draw(3, 1, 0, 0);
		commandBuffers[i].endRenderPass();

		//commandBuffers[i].end();

		res = vk::Result(vkEndCommandBuffer(commandBuffers[i]));

		if (res != vk::Result::eSuccess) {
			throw std::runtime_error(std::string("VK_EndCommandBuffer Error: ") + vk::to_string(res));
		}
	}

	vk::SemaphoreCreateInfo semaphoreInfo = vk::SemaphoreCreateInfo(vk::SemaphoreCreateFlags());

	res = device.createSemaphore(&semaphoreInfo, nullptr, &imageAvailable);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateSemaphore Error: ") + vk::to_string(res));
	}

	res = device.createSemaphore(&semaphoreInfo, nullptr, &renderFinished);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_CreateSemaphore Error: ") + vk::to_string(res));
	}
}

void drawFrame() {
	// Update engine state

	presentationQueue.waitIdle();

	uint32_t imageIndex;

	vk::Result res = device.acquireNextImageKHR(swapChain, std::numeric_limits<uint64_t>::max(), imageAvailable, nullptr, &imageIndex);

	if (res == vk::Result::eErrorOutOfDateKHR) {
		throw std::runtime_error("TO DO: Handle out of date swapchain");
	}
	else if (res != vk::Result::eSuccess && res != vk::Result::eSuboptimalKHR) {
		throw std::runtime_error(std::string("VK_AcquireNextImageKHR Error: ") + vk::to_string(res));
	}

	vk::PipelineStageFlags waitStages[] = {
		vk::PipelineStageFlagBits::eColorAttachmentOutput
	};

	vk::SubmitInfo submitInfo(1, &imageAvailable, waitStages, 1, &commandBuffers[imageIndex], 1, &renderFinished);

	res = graphicsQueue.submit(1, &submitInfo, nullptr);

	if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_QueueSubmit Error: ") + vk::to_string(res));
	}

	vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR(1, &renderFinished, 1, &swapChain, &imageIndex, nullptr);

	res = presentationQueue.presentKHR(&presentInfo);

	if (res == vk::Result::eErrorOutOfDateKHR || res == vk::Result::eSuboptimalKHR) {
		throw std::runtime_error("TO DO: Handle out of date swapchain");
	}
	else if (res != vk::Result::eSuccess) {
		throw std::runtime_error(std::string("VK_QueuePresentKHR Error: ") + vk::to_string(res));
	}

	presentationQueue.waitIdle();
}

void update() {
	SDL_Event event;

	bool quit = false;

	while (!quit) {
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				quit = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_ESCAPE) {
					quit = true;
				}
			}
		}

		drawFrame();
	}

	device.waitIdle();
}

void cleanUp() {
	device.destroySemaphore(imageAvailable, nullptr);
	device.destroySemaphore(renderFinished, nullptr);

	device.destroyCommandPool(commandPool, nullptr);

	for (const vk::Framebuffer framebuffer : swapChainFramebuffers) {
		device.destroyFramebuffer(framebuffer, nullptr);
	}

	device.destroyPipeline(graphicsPipeline, nullptr);
	device.destroyPipelineLayout(pipelineLayout, nullptr);

	device.destroyRenderPass(renderPass, nullptr);

	for (const vk::ImageView view : swapChainImageViews) {
		device.destroyImageView(view, nullptr);
	}

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
		cleanUp();
	}
	catch (std::runtime_error e) {
		std::cerr << e.what() << std::endl;
	}

#ifdef _WIN32
	system("pause");
#endif

	return 0;
}