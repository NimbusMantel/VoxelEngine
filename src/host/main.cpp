#include <SDL2/SDL.h>
#include <GL/glew.h>

#include <CL/cl.hpp>

#include <map>
#include <vector>
#include <fstream>

#include <iostream>

#include <Windows.h>

#include "host/camera.hpp"
#include "voxel/voxel.hpp"
#include "voxel/material.hpp"

#include <random>

#define OpenCLDebug 0

bool HOST_BIG_ENDIAN, DEVICE_BIG_ENDIAN;

const uint16_t width = 640;
const uint16_t height = 360;

const uint16_t size = min(width, height);

const uint8_t fov = 70;

const float rayCoef = width * 0.56875f;

uint32_t tmp = (127 - VOXEL_DEPTH) << 23;

const float scale = *((float*)(&tmp));

uint32_t timeStamp = 0x00;

float pos[3], rot[9];
float vel[3] = { 0.0f, 0.0f, 0.0f };

#define blArFw_0 width
#define blArFh_0 height

#define blArFw_1 (cl_int)ceilf(blArFw_0 * 0.5f)
#define blArFh_1 (cl_int)ceilf(blArFh_0 * 0.5f)

#define blArFw_2 (cl_int)ceilf(blArFw_1 * 0.5f)
#define blArFh_2 (cl_int)ceilf(blArFh_1 * 0.5f)

#define blArFw_3 (cl_int)ceilf(blArFw_2 * 0.5f)
#define blArFh_3 (cl_int)ceilf(blArFh_2 * 0.5f)

#define blArFw_4 (cl_int)ceilf(blArFw_3 * 0.5f)
#define blArFh_4 (cl_int)ceilf(blArFh_3 * 0.5f)

#define blArFw_5 (cl_int)ceilf(blArFw_4 * 0.5f)
#define blArFh_5 (cl_int)ceilf(blArFh_4 * 0.5f)

const cl_int4 bloomAreas[6] = {	{ 0, 0, blArFw_0 - 1, blArFh_0 - 1 },
								{ 0, blArFh_0, blArFw_1 - 1, blArFh_0 + blArFh_1 - 1 },
								{ blArFw_1, blArFh_0, blArFw_1 + blArFw_2 - 1, blArFh_0 + blArFh_2 - 1 },
								{ blArFw_1 + blArFw_2, blArFh_0, blArFw_1 + blArFw_2 + blArFw_3 - 1, blArFh_0 + blArFh_3 - 1 },
								{ blArFw_1 + blArFw_2 + blArFw_3, blArFh_0, blArFw_1 + blArFw_2 + blArFw_3 + blArFw_4 - 1, blArFh_0 + blArFh_4 - 1 },
								{ blArFw_1 + blArFw_2 + blArFw_3 + blArFw_4, blArFh_0, blArFw_1 + blArFw_2 + blArFw_3 + blArFw_4 + blArFw_5 - 1, blArFh_0 + blArFh_5 - 1 }
};

uint16_t mousePosX = 0;
uint32_t mousePosY = 0;

bool isDragging = false;
bool isFullscreen = false;

uint16_t fullWidth;
uint16_t fullHeight;

void testVoxelBinaryTree();
void testVoxelBufferData();

int main(int argc, char* argv[]) {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) return -1;
	
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	char titat[] = "Voxel Engine (%d fps)";
	char title[32];

	SDL_DisplayMode mode;
	SDL_GetDesktopDisplayMode(0, &mode);

	fullWidth = mode.w;
	fullHeight = mode.h;

	SDL_Window* window = SDL_CreateWindow("Voxel Engine (0 fps)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, fullWidth, fullHeight, SDL_WINDOW_OPENGL);
	SDL_SetWindowSize(window, width, height);
	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	
	if (window == NULL) return -1;

	SDL_GLContext context = SDL_GL_CreateContext(window);

	if (context == NULL) return -1;

	SDL_GL_SetSwapInterval(1);

	glewInit();

	GLuint fbo, rbo;

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClearColor(0, 0, 0, 0);

	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);

	cl::Platform platform = platforms.front();

	std::vector<cl::Device> devices;
	platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

	cl::Device device = devices.front();

	union { uint32_t i; char c[4]; } bint = { 0x01020304 }; HOST_BIG_ENDIAN = (bint.c[0] == 0x01);
	device.getInfo(CL_DEVICE_ENDIAN_LITTLE, &DEVICE_BIG_ENDIAN); DEVICE_BIG_ENDIAN = !DEVICE_BIG_ENDIAN;

	cl_context_properties props[] =
	{
		CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
		CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
		0
	};

	cl::Context clContext = cl::Context(device, props);
	
	cl::BufferRenderGL ldrImage = cl::BufferRenderGL(clContext, CL_MEM_READ_WRITE, rbo);
	glFinish();
	std::vector<cl::Memory> glMemory = { ldrImage };

	cl::Buffer vxBuffer = cl::Buffer(clContext, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, 0x80 << BUFFER_DEPTH);

	cl::Image2D hdrImage = cl::Image2D(clContext, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, { CL_RGBA, CL_HALF_FLOAT }, width, height);
	cl::Image2D blmImage = cl::Image2D(clContext, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, { CL_RGBA, CL_HALF_FLOAT }, width, height * 1.5f);

	cl::Buffer cgBuffer = cl::Buffer(clContext, (cl_mem_flags)(CL_MEM_READ_WRITE), 0x01 << 24);

	cl::Buffer mnTicket = cl::Buffer(clContext, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, 0x04 << 1);
	cl::Buffer mnBuffer = cl::Buffer(clContext, CL_MEM_READ_WRITE | CL_MEM_HOST_WRITE_ONLY, 0x08 << BUFFER_DEPTH);

	cl::Buffer rvLookup = cl::Buffer(clContext, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, sizeof(cl_float) * 3 * width * height);
	cl::Buffer rtMatrix = cl::Buffer(clContext, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(cl_float) * 9);

	std::ifstream strm("src/kernel/kernel.cl");

	std::string kstr(std::istreambuf_iterator<char>(strm), (std::istreambuf_iterator<char>()));
	
	std::string rstr = "";

	while (kstr.find("/*KERNEL_EXCLUDE_BEG*/") != -1) {
		kstr.replace(kstr.find("/*KERNEL_EXCLUDE_BEG*/"), kstr.find("/*KERNEL_EXCLUDE_END*/") - kstr.find("/*KERNEL_EXCLUDE_BEG*/") + 22, rstr);
	}

	strm = std::ifstream("src/kernel/instruct.cpp");
	rstr = std::string(std::istreambuf_iterator<char>(strm), (std::istreambuf_iterator<char>()));
	rstr = rstr.substr(rstr.find("/*KERNEL_INSTRUCT_BEG*/") + 23, rstr.find("/*KERNEL_INSTRUCT_END*/") - rstr.find("/*KERNEL_INSTRUCT_BEG*/") - 23);
	kstr.replace(kstr.find("/*KERNEL_INSTRUCT_BEG*/"), kstr.find("/*KERNEL_INSTRUCT_END*/") - kstr.find("/*KERNEL_INSTRUCT_BEG*/") + 23, rstr);

	strm = std::ifstream("src/voxel/material.hpp");
	rstr = std::string(std::istreambuf_iterator<char>(strm), (std::istreambuf_iterator<char>()));
	rstr = rstr.substr(rstr.find("/*KERNEL_BETSTOPS_BEG*/") + 23, rstr.find("/*KERNEL_BETSTOPS_END*/") - rstr.find("/*KERNEL_BETSTOPS_BEG*/") - 23);
	kstr.replace(kstr.find("/*KERNEL_BETSTOPS_BEG*/"), kstr.find("/*KERNEL_BETSTOPS_END*/") - kstr.find("/*KERNEL_BETSTOPS_BEG*/") + 23, rstr);

	cl::Program::Sources sources = cl::Program::Sources(1, std::make_pair(kstr.c_str(), kstr.length() + 1));

	cl::Program clProgram(clContext, sources);

	char* clBuild = new char[30]();

	sprintf(clBuild, "-cl-std=CL1.2 -D OpenCLDebug=%u", (bool)OpenCLDebug);
	
	clProgram.build(clBuild);

	cl::Kernel rayInitKernel = cl::Kernel(clProgram, "rayInitKernel");
	rayInitKernel.setArg(0, rvLookup);
	
	cl::Kernel cgProKernel = cl::Kernel(clProgram, "cgProKernel");
	cgProKernel.setArg(0, vxBuffer);
	cgProKernel.setArg(1, mnTicket);
	cgProKernel.setArg(2, mnBuffer);
	cgProKernel.setArg(3, cgBuffer);

	cl::Kernel hdrRenKernel = cl::Kernel(clProgram, "hdrRenKernel");
	hdrRenKernel.setArg(0, vxBuffer);
	hdrRenKernel.setArg(1, mnTicket);
	hdrRenKernel.setArg(2, mnBuffer);
	hdrRenKernel.setArg(3, cgBuffer);
	hdrRenKernel.setArg(4, hdrImage);
	hdrRenKernel.setArg(5, rvLookup);
	hdrRenKernel.setArg(6, rtMatrix);
	hdrRenKernel.setArg(8, rayCoef);

	cl::Kernel linBlurKernel = cl::Kernel(clProgram, "linBlurKernel");
	linBlurKernel.setArg(1, blmImage);

	cl::Kernel dwSampKernel = cl::Kernel(clProgram, "dwSampKernel");
	dwSampKernel.setArg(0, blmImage);
	dwSampKernel.setArg(1, blmImage);

	cl::Kernel upSampKernel = cl::Kernel(clProgram, "upSampKernel");
	upSampKernel.setArg(0, blmImage);
	upSampKernel.setArg(1, blmImage);

	cl::Kernel bloomKernel = cl::Kernel(clProgram, "bloomKernel");
	bloomKernel.setArg(0, hdrImage);
	bloomKernel.setArg(1, blmImage);
	bloomKernel.setArg(2, hdrImage);
	bloomKernel.setArg(3, blmImage);

	cl::Kernel linLumKernel = cl::Kernel(clProgram, "linLumKernel");
	linLumKernel.setArg(0, blmImage);
	linLumKernel.setArg(1, blmImage);

	cl::Kernel hdrExpKernel = cl::Kernel(clProgram, "hdrExpKernel");
	hdrExpKernel.setArg(0, hdrImage);
	hdrExpKernel.setArg(1, blmImage);
	hdrExpKernel.setArg(2, ldrImage);

	cl::Kernel gcReqKernel = cl::Kernel(clProgram, "gcReqKernel");
	gcReqKernel.setArg(0, vxBuffer);
	gcReqKernel.setArg(1, mnTicket);
	gcReqKernel.setArg(2, mnBuffer);
	gcReqKernel.setArg(3, cgBuffer);

	cl::Kernel gcSugKernel = cl::Kernel(clProgram, "gcSugKernel");
	gcSugKernel.setArg(0, vxBuffer);
	gcSugKernel.setArg(1, mnTicket);
	gcSugKernel.setArg(2, mnBuffer);
	gcSugKernel.setArg(3, cgBuffer);
	
	cl::CommandQueue clQueue = cl::CommandQueue(clContext, device);

	uint8_t* cgBuf = manCTG::buf();
	uint32_t cgBufSize;

	uint32_t cgInsSyncAmount = 0;
	uint32_t cgInsAsyncAmount = 0;
	uint32_t cgInsSumAmount;
	
	cl_float3 norPos;

	cl_int2 step;
	
	uint8_t div;

	int currentFrame, previousFrame = SDL_GetTicks(), fps;

	bool quit = false;

	SDL_Event event;

	// Init ray direction vectors

	{
		float fovRad = (M_PI * fov) / 360.0f;
		float hwRat = height / (float)width;
		float halfW = tanf(fovRad);
		float halfH = hwRat * halfW;
		float pixelW = (halfW * 2.0f) / (float)(width - 1);
		float pixelH = (halfH * 2.0f) / (float)(height - 1);
		
		rayInitKernel.setArg(1, pixelW);
		rayInitKernel.setArg(2, pixelH);
		rayInitKernel.setArg(3, halfW);
		rayInitKernel.setArg(4, halfH);
		rayInitKernel.setArg(5, width);
		
		for (uint16_t h = 0; h < height; h += min(32, height - h)) {
			for (uint16_t w = 0; w < width; w += min(32, width - w)) {
				clQueue.enqueueNDRangeKernel(rayInitKernel, cl::NDRange(w, h), cl::NDRange(min(32, width - w), min(32, height - h)), cl::NullRange);
			}
		}

		clQueue.finish();
	}

	// Init voxel buffer by inserting the root

	{
		clQueue.enqueueFillBuffer(mnBuffer, 0x00, 0, 8);
		
		manBuf::set(0, true);

		manVox::init();

		cgBufSize = manCTG::wri(cgInsSyncAmount, cgInsAsyncAmount);
		cgInsSumAmount = cgInsSyncAmount + cgInsAsyncAmount;

		clQueue.enqueueWriteBuffer(cgBuffer, true, 0, cgBufSize, (void*)cgBuf);

		clQueue.enqueueBarrierWithWaitList();

		cgProKernel.setArg(4, cgInsSyncAmount);
		cgProKernel.setArg(5, cgInsAsyncAmount);

		for (uint32_t i = 0; i < cgInsSumAmount; i += min(1024, cgInsSumAmount - i)) {
			clQueue.enqueueNDRangeKernel(cgProKernel, cl::NDRange(i), cl::NDRange(min(1024, cgInsSumAmount - i)), cl::NullRange);
		}

		clQueue.finish();
	}

	testVoxelBufferData();

	while (!quit) {
		// Enqueue CPU to GPU instrcutions
		
		cgBufSize = manCTG::wri(cgInsSyncAmount, cgInsAsyncAmount);
		cgInsSumAmount = cgInsSyncAmount + cgInsAsyncAmount;

		if (cgBufSize > 0) {
			clQueue.enqueueWriteBuffer(cgBuffer, true, 0, cgBufSize, (void*)cgBuf);

			clQueue.enqueueBarrierWithWaitList();

			cgProKernel.setArg(4, cgInsSyncAmount);
			cgProKernel.setArg(5, cgInsAsyncAmount);

			for (uint32_t i = 0; i < cgInsSumAmount; i += min(1024, cgInsSumAmount - i)) {
				clQueue.enqueueNDRangeKernel(cgProKernel, cl::NDRange(i), cl::NDRange(min(1024, cgInsSumAmount - i)), cl::NullRange);
			}
		}

		clQueue.enqueueBarrierWithWaitList();

		clQueue.enqueueFillBuffer(cgBuffer, 0x00, 0, 21);
		clQueue.finish();

		glClear(GL_COLOR_BUFFER_BIT);
		glFinish();

		clQueue.enqueueAcquireGLObjects(&glMemory);

		camera::mat(rot);

		clQueue.enqueueWriteBuffer(rtMatrix, true, 0, sizeof(cl_float) * 9, (void*)rot);

		clQueue.enqueueBarrierWithWaitList();

		camera::pos(pos[0], pos[1], pos[2]);

		norPos.x = pos[0] * scale + 1.5f;
		norPos.y = pos[1] * scale + 1.5f;
		norPos.z = pos[2] * scale + 1.5f;

		hdrRenKernel.setArg(7, norPos);

		timeStamp = (timeStamp + 1) & 0x0000003F;

		hdrRenKernel.setArg(9, timeStamp);
		
		for (uint16_t h = 0; h < height; h += min(32, height - h)) {
			for (uint16_t w = 0; w < width; w += min(32, width - w)) {
				clQueue.enqueueNDRangeKernel(hdrRenKernel, cl::NDRange(w, h), cl::NDRange(min(32, width - w), min(32, height - h)), cl::NullRange);
			}
		}

		clQueue.enqueueBarrierWithWaitList();

		step = { 1, 0 };

		linBlurKernel.setArg(0, hdrImage);
		linBlurKernel.setArg(2, bloomAreas[0].lo);
		linBlurKernel.setArg(3, step);
		linBlurKernel.setArg(4, width);

		for (uint16_t h = 0; h < height; h += (((height - h) > 32) ? (min(32, (height - h) / 32) * 32) : (height - h))) {
			clQueue.enqueueNDRangeKernel(linBlurKernel, cl::NDRange(0, h / 32), cl::NDRange(min(32, height - h), ((height - h) > 32) ? min(32, (height - h) / 32) : 1), cl::NullRange);
		}

		clQueue.enqueueBarrierWithWaitList();

		step = { 0, 1 };

		linBlurKernel.setArg(0, blmImage);
		linBlurKernel.setArg(3, step);
		linBlurKernel.setArg(4, height);

		for (uint16_t w = 0; w < width; w += (((width - w) > 32) ? (min(32, (width - w) / 32) * 32) : (width - w))) {
			clQueue.enqueueNDRangeKernel(linBlurKernel, cl::NDRange(0, w / 32), cl::NDRange(min(32, width - w), ((width - w) > 32) ? min(32, (width - w) / 32) : 1), cl::NullRange);
		}

		clQueue.enqueueBarrierWithWaitList();

		div = 1;

		for (uint8_t i = 0; i < 5; ++i) {
			div <<= 1;

			dwSampKernel.setArg(2, bloomAreas[i].lo);
			dwSampKernel.setArg(3, bloomAreas[i + 1].lo);

			for (uint16_t h = 0; h < (height / div); h += min(32, (height / div) - h)) {
				for (uint16_t w = 0; w < (width / div); w += min(32, (width / div) - w)) {
					clQueue.enqueueNDRangeKernel(dwSampKernel, cl::NDRange(w, h), cl::NDRange(min(32, (width / div) - w), min(32, (height / div) - h)), cl::NullRange);
				}
			}

			clQueue.enqueueBarrierWithWaitList();

			step = { 1, 0 };

			linBlurKernel.setArg(2, bloomAreas[i + 1].lo);
			linBlurKernel.setArg(3, step);
			linBlurKernel.setArg(4, (uint16_t)(width / div));

			for (uint16_t h = 0; h < (height / div); h += ((((height / div) - h) > 32) ? (min(32, ((height / div) - h) / 32) * 32) : ((height / div) - h))) {
				clQueue.enqueueNDRangeKernel(linBlurKernel, cl::NDRange(0, h / 32), cl::NDRange(min(32, (height / div) - h), (((height / div) - h) > 32) ? min(32, ((height / div) - h) / 32) : 1), cl::NullRange);
			}

			clQueue.enqueueBarrierWithWaitList();

			step = { 0, 1 };

			linBlurKernel.setArg(3, step);
			linBlurKernel.setArg(4, (uint16_t)(height / div));

			for (uint16_t w = 0; w < (width / div); w += ((((width / div) - w) > 32) ? (min(32, ((width / div) - w) / 32) * 32) : ((width / div) - w))) {
				clQueue.enqueueNDRangeKernel(linBlurKernel, cl::NDRange(0, w / 32), cl::NDRange(min(32, (width / div) - w), (((width / div) - w) > 32) ? min(32, ((width / div) - w) / 32) : 1), cl::NullRange);
			}

			clQueue.enqueueBarrierWithWaitList();
		}

		for (uint8_t i = 0; i < 5; ++i) {
			div >>= 1;
			
			upSampKernel.setArg(2, bloomAreas[5 - i].lo);
			upSampKernel.setArg(3, bloomAreas[4 - i].lo);
			upSampKernel.setArg(4, bloomAreas[5 - i].hi);

			for (uint16_t h = 0; h < (height / div); h += min(32, (height / div) - h)) {
				for (uint16_t w = 0; w < (width / div); w += min(32, (width / div) - w)) {
					clQueue.enqueueNDRangeKernel(upSampKernel, cl::NDRange(w, h), cl::NDRange(min(32, (width / div) - w), min(32, (height / div) - h)), cl::NullRange);
				}
			}

			clQueue.enqueueBarrierWithWaitList();
		}

		for (uint16_t h = 0; h < height; h += min(32, height - h)) {
			for (uint16_t w = 0; w < width; w += min(32, width - w)) {
				clQueue.enqueueNDRangeKernel(bloomKernel, cl::NDRange(w, h), cl::NDRange(min(32, width - w), min(32, height - h)), cl::NullRange);
			}
		}

		clQueue.enqueueBarrierWithWaitList();

		step = { 1, 0 };

		linLumKernel.setArg(2, step);
		linLumKernel.setArg(3, width);

		for (uint16_t h = 0; h < height; h += (((height - h) > 32) ? (min(32, (height - h) / 32) * 32) : (height - h))) {
			clQueue.enqueueNDRangeKernel(linLumKernel, cl::NDRange(0, h / 32), cl::NDRange(min(32, height - h), ((height - h) > 32) ? min(32, (height - h) / 32) : 1), cl::NullRange);
		}

		clQueue.enqueueBarrierWithWaitList();

		step = { 0, 1 };

		linLumKernel.setArg(2, step);
		linLumKernel.setArg(3, height);

		for (uint16_t w = 0; w < width; w += (((width - w) > 32) ? (min(32, (width - w) / 32) * 32) : (width - w))) {
			clQueue.enqueueNDRangeKernel(linLumKernel, cl::NDRange(0, w / 32), cl::NDRange(min(32, width - w), ((width - w) > 32) ? min(32, (width - w) / 32) : 1), cl::NullRange);
		}

		clQueue.enqueueBarrierWithWaitList();

		for (uint16_t h = 0; h < height; h += min(32, height - h)) {
			for (uint16_t w = 0; w < width; w += min(32, width - w)) {
				clQueue.enqueueNDRangeKernel(hdrExpKernel, cl::NDRange(w, h), cl::NDRange(min(32, width - w), min(32, height - h)), cl::NullRange);
			}
		}

		clQueue.enqueueBarrierWithWaitList();

		clQueue.enqueueReleaseGLObjects(&glMemory);
		clQueue.finish();

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glDrawBuffer(GL_BACK);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
		glReadBuffer(GL_COLOR_ATTACHMENT0);

		glBlitFramebuffer(0, 0, width, height, 0, 0, isFullscreen ? fullWidth : width, isFullscreen ? fullHeight : height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		
		gcReqKernel.setArg(4, timeStamp);
		gcReqKernel.setArg(5, manBuf::per());

		gcSugKernel.setArg(4, timeStamp);
		gcSugKernel.setArg(5, manBuf::per());

		clQueue.enqueueTask(gcReqKernel);
		
		clQueue.enqueueBarrierWithWaitList();

		clQueue.enqueueReadBuffer(cgBuffer, true, 0, 1, (void*)cgBuf);

		while (cgBuf[0] != 0xFF) {
			clQueue.enqueueNDRangeKernel(gcSugKernel, cl::NullRange, cl::NDRange(16), cl::NDRange(16));

			clQueue.enqueueBarrierWithWaitList();

			clQueue.enqueueReadBuffer(cgBuffer, true, 0, 1, (void*)cgBuf);
		}
		
		clQueue.enqueueReadBuffer(cgBuffer, true, 1, 3, (void*)cgBuf);

		cgBufSize = ((cgBuf[0] << 16) | (cgBuf[1] << 8) | cgBuf[2]);

		if (cgBufSize > 0) {
			clQueue.enqueueReadBuffer(cgBuffer, true, 4, cgBufSize, (void*)cgBuf);

			// TO DO: Evaluate GPU to CPU communication
		}

		SDL_GL_SwapWindow(window);

		currentFrame = SDL_GetTicks();
		fps = (1000 / ((currentFrame - previousFrame) | (currentFrame == previousFrame)));
		sprintf(title, titat, fps);
		SDL_SetWindowTitle(window, title);
		previousFrame = currentFrame;

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				quit = true;
			}
			else if (event.type == SDL_KEYDOWN) {
				if (event.key.keysym.sym == SDLK_ESCAPE) {
					if (isFullscreen) {
						isFullscreen = false;

						SDL_SetWindowFullscreen(window, 0);
					}
					else {
						quit = true;
					}
				}
				else if (event.key.keysym.sym == SDLK_f) {
					isFullscreen = !isFullscreen;

					SDL_SetWindowFullscreen(window, isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
				}
				else if (event.key.keysym.sym == SDLK_c) {
					if (camera::mod() == camera::Mode::FREE) {
						camera::mod(camera::Mode::ARCBALL);

						if (!isDragging) {
							SDL_SetRelativeMouseMode(SDL_FALSE);
						}
					}
					else if (camera::mod() == camera::Mode::ARCBALL) {
						camera::mod(camera::Mode::FREE);

						SDL_SetRelativeMouseMode(SDL_TRUE);
					}
				}
				else if (event.key.keysym.sym == SDLK_UP) {
					vel[2] = 0.1f;
				}
				else if (event.key.keysym.sym == SDLK_DOWN) {
					vel[2] = -0.1f;
				}
				else if (event.key.keysym.sym == SDLK_LEFT) {
					vel[0] = -0.1f;
				}
				else if (event.key.keysym.sym == SDLK_RIGHT) {
					vel[0] = 0.1f;
				}
				else if (event.key.keysym.sym == SDLK_HOME) {
					vel[1] = 0.1f;
				}
				else if (event.key.keysym.sym == SDLK_END) {
					vel[1] = -0.1f;
				}
				else if (event.key.keysym.sym == SDLK_w) {
					camera::rot(0.0f, 0.01f);
				}
				else if (event.key.keysym.sym == SDLK_a) {
					camera::rot(-0.01f, 0.0f);
				}
				else if (event.key.keysym.sym == SDLK_s) {
					camera::rot(0.0f, -0.01f);
				}
				else if (event.key.keysym.sym == SDLK_d) {
					camera::rot(0.01f, 0.0f);
				}
			}
			else if (event.type == SDL_KEYUP) {
				if (event.key.keysym.sym == SDLK_UP) {
					vel[2] = 0.0f;
				}
				else if (event.key.keysym.sym == SDLK_DOWN) {
					vel[2] = 0.0f;
				}
				else if (event.key.keysym.sym == SDLK_LEFT) {
					vel[0] = 0.0f;
				}
				else if (event.key.keysym.sym == SDLK_RIGHT) {
					vel[0] = 0.0f;
				}
				else if (event.key.keysym.sym == SDLK_HOME) {
					vel[1] = 0.0f;
				}
				else if (event.key.keysym.sym == SDLK_END) {
					vel[1] = 0.0f;
				}
			}
			else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
				isDragging = true;
				
				mousePosX = event.button.x;
				mousePosY = height - 1 - event.button.y;

				SDL_SetRelativeMouseMode(SDL_TRUE);
			}
			else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
				isDragging = false;

				mousePosX = event.button.x;
				mousePosY = height - 1 - event.button.y;

				if (camera::mod() != camera::Mode::FREE) {
					SDL_SetRelativeMouseMode(SDL_FALSE);
				}
			}
			else if (event.type == SDL_MOUSEMOTION) {
				mousePosX = event.motion.x;
				mousePosY = height - 1 - event.motion.y;

				if (camera::mod() == camera::Mode::FREE || (camera::mod() == camera::Mode::ARCBALL && isDragging)) {
					camera::rot(0.02f * event.motion.xrel / (0.5f * size), -0.02f * event.motion.yrel / (0.5f * size));
				}
			}
			else if (event.type == SDL_MOUSEWHEEL) {
				if (camera::mod() == camera::Mode::ARCBALL) {
					camera::rad(event.wheel.y * -0.5f);
				}
			}
		}
		
		if (vel[0] != 0.0f || vel[1] != 0.0f || vel[2] != 0.0f) {
			camera::mov(vel[0], vel[1], vel[2]);
		}
	}

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

void testVoxelBinaryTree() {
	std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_int_distribution<int> uni(0, (0x01 << BUFFER_DEPTH) - 1);

	std::cout << "Set Random" << std::endl << std::endl;
	for (int i = 0; i < 64; ++i) {
		manBuf::set(uni(rng), true);
	}
	manBuf::dis();

	std::cout << std::endl << std::endl << "Set All" << std::endl << std::endl;
	manBuf::set(0, 0x01 << BUFFER_DEPTH, true);
	manBuf::dis();

	std::cout << std::endl << std::endl << "Set Gap" << std::endl << std::endl;
	manBuf::set(2, (0x01 << BUFFER_DEPTH) - 4, false);
	manBuf::dis();

	std::cout << std::endl << std::endl << "Set Block" << std::endl << std::endl;
	manBuf::set(11, 18, true);
	manBuf::set(39, 7, true);
	manBuf::dis();

	std::cout << std::endl << std::endl << "Allocation" << std::endl << std::endl;
	std::vector<std::pair<uint32_t, uint32_t>> rs;
	manBuf::alo(23, rs);
	for (std::vector<std::pair<uint32_t, uint32_t>>::iterator it = rs.begin(); it != rs.end(); ++it) {
		std::cout << "(" << it->first << ": " << it->second << ") ";
	}
	std::cout << std::endl;

	std::cout << std::endl;
	for (std::vector<std::pair<uint32_t, uint32_t>>::iterator it = rs.begin(); it != rs.end(); ++it) {
		manBuf::set(it->first, it->second, true);
	}
	manBuf::dis();

	std::cout << std::endl;
}

void testVoxelBufferData() {
	manBuf::set(1, true, 16);

	vis viss[VOXEL_DEPTH] = { vis() };

	mat vxs[8] = { mat(col(1.0f, 0.929f, 0.0f), 0.875f, col(1.0f, 0.929f, 0.0f)), mat(col(1.0f, 0.0f, 0.0f), 0.75f, col(1.0f, 0.0f, 0.0f)),
				   mat(col(1.0f, 0.0f, 0.671f), 0.625f, col(1.0f, 0.0f, 0.671f)), mat(col(0.0f, 0.278f, 0.671f), 0.5f, col(0.0f, 0.278f, 0.671f)),
				   mat(col(0.0f, 0.929f, 1.0f), 0.375f, col(0.0f, 0.929f, 1.0f)), mat(col(0.0f, 0.710f, 0.0f), 0.25f, col(0.0f, 0.710f, 0.0f)),
				   mat(col(1.0f, 1.0f, 1.0f), 1.0f, col(0.0f, 0.0f, 0.0f)), mat(col(0.0f, 0.0f, 0.0f), 0.125f, col(0.25f, 0.25f, 0.25f))
	};

	viss[VOXEL_DEPTH - 1] = vis(mat::abstract(vxs), new mLit(col(1.0f, 1.0f, 1.0f), 1.0f));

	mat abt[8] = { mat() };

	for (int i = (VOXEL_DEPTH - 2); i >= 0; --i) {
		abt[0] = viss[i + 1].mal;

		viss[i].mal = mat::abstract(abt);
		viss[i].lig.reset(new mLit(col(1.0f, 1.0f, 1.0f), 0.0f));
	}

	std::unique_ptr<INS_CTG> t;

	uint64_t tmp = viss[0].toBinary();

	t.reset(new INS_CTG_MAT(0, tmp >> 32));
	manCTG::eqA(std::move(t));

	t.reset(new INS_CTG_LIT(0, tmp & 0xFFFFFFFF));
	manCTG::eqA(std::move(t));

	uint32_t par = 0;
	uint32_t idx = 8;

	t.reset(new INS_CTG_EXP(par, idx));
	manCTG::eqS(std::move(t));

	par = 15;

	for (int i = 2; i <= VOXEL_DEPTH; ++i) {
		idx = 8 * i;
		
		t.reset(new INS_CTG_EXP(par, idx));
		manCTG::eqS(std::move(t));

		par = 8 * i;
	}

	uint32_t o[8] = { 0x00 };
	std::unique_ptr<uint32_t[]> v(new uint32_t[8]());

	par = 0;

	tmp = viss[1].toBinary();

	o[0] = 0xF0000000;
	o[1] = 16;
	o[2] = tmp >> 32;
	o[3] = tmp & 0xFFFFFFFF;

	v.reset(new uint32_t[8]());
	memcpy(v.get(), &o, 8 * 4);
	t.reset(new INS_CTG_ADD(par, std::move(v)));
	manCTG::eqS(std::move(t));

	par = 15;

	for (int i = 2; i <= (VOXEL_DEPTH - 1); ++i) {
		tmp = viss[i].toBinary();

		o[0] = 0x80000000 | ((i - 1) << 24);
		o[1] = 8 * (i + 1);
		o[2] = tmp >> 32;
		o[3] = tmp & 0xFFFFFFFF;
		
		v.reset(new uint32_t[8]());
		memcpy(v.get(), &o, 8 * 4);
		t.reset(new INS_CTG_ADD(par, std::move(v)));
		manCTG::eqS(std::move(t));

		par = 8 * i;
	}

	for (uint32_t i = 0; i < 4; ++i) {
		tmp = vis(vxs[i * 2], new mLit(col(1.0f, 1.0f, 1.0f), (i == 0) ? 100.0f : 0.5f)).toBinary();

		o[0] = 0x80000000 | (i << 29) | ((i != 0) ? (((par & (0xF0000000 >> (i * 8))) >> (28 - i * 8)) << 24) : (15 << 24));
		o[1] = 0x00000000;
		o[2] = tmp >> 32;
		o[3] = tmp & 0xFFFFFFFF;

		tmp = vis(vxs[i * 2 + 1], new mLit(col(1.0f, 1.0f, 1.0f), 0.5f)).toBinary();

		o[4] = 0x90000000 | (i << 29) | (((par & (0x0F000000 >> (i * 8))) >> (24 - i * 8)) << 24);
		o[5] = 0x00000000;
		o[6] = tmp >> 32;
		o[7] = tmp & 0xFFFFFFFF;
		
		v.reset(new uint32_t[8]());
		memcpy(v.get(), &o, 8 * 4);
		t.reset(new INS_CTG_ADD(par, std::move(v)));
		manCTG::eqS(std::move(t));
	}

	camera::mov(1.0f, 1.0f, -1.0f);
	camera::rad(10.0f);
}