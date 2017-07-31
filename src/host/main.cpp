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

float pos[3], rot[9];
float vel[3] = { 0.0f, 0.0f, 0.0f };

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
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
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
	
	cl::BufferRenderGL glBuffer = cl::BufferRenderGL(clContext, CL_MEM_WRITE_ONLY, rbo);
	glFinish();
	std::vector<cl::Memory> glMemory = { glBuffer };

	cl::Buffer vxBuffer = cl::Buffer(clContext, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, 0x80 << BUFFER_DEPTH);

	cl::Buffer cgBuffer = cl::Buffer(clContext, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, 0x01 << 24);
	cl::Buffer gcBuffer = cl::Buffer(clContext, CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY, 0x01 << 16);

	cl::Buffer rvLookup = cl::Buffer(clContext, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, sizeof(cl_float) * 3 * width * height);
	cl::Buffer ldLookup = cl::Buffer(clContext, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, 64);
	cl::Buffer rtMatrix = cl::Buffer(clContext, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, sizeof(cl_float) * 9);

	std::ifstream istrm("src/kernel/instruct.cpp");
	std::string isrc(std::istreambuf_iterator<char>(istrm), (std::istreambuf_iterator<char>()));
	isrc = isrc.substr(isrc.find("/*KERNEL_INCLUDE_BEG*/") + 22, isrc.find("/*KERNEL_INCLUDE_END*/") - isrc.find("/*KERNEL_INCLUDE_BEG*/") - 22);

	std::ifstream strm("src/kernel/kernel.cl");
	std::string src(std::istreambuf_iterator<char>(strm), (std::istreambuf_iterator<char>()));
	src.replace(src.find("/*KERNEL_INCLUDE_BEG*/"), src.find("/*KERNEL_INCLUDE_END*/") - src.find("/*KERNEL_INCLUDE_BEG*/") + 22, isrc);
	
	cl::Program::Sources sources = cl::Program::Sources(1, std::make_pair(src.c_str(), src.length() + 1));

	cl::Program clProgram(clContext, sources);

	char* clBuild = new char[30]();

	sprintf(clBuild, "-cl-std=CL1.2 -D OpenCLDebug=%u", (bool)OpenCLDebug);
	
	clProgram.build(clBuild);

	cl::Kernel rayInitKernel = cl::Kernel(clProgram, "rayInitKernel");
	rayInitKernel.setArg(0, rvLookup);
	
	cl::Kernel cgProKernel = cl::Kernel(clProgram, "cgProKernel");
	cgProKernel.setArg(0, vxBuffer);
	cgProKernel.setArg(1, cgBuffer);

	cl::Kernel renderKernel = cl::Kernel(clProgram, "renderKernel");
	renderKernel.setArg(0, vxBuffer);
	renderKernel.setArg(1, glBuffer);
	renderKernel.setArg(2, ldLookup);
	renderKernel.setArg(3, rvLookup);
	renderKernel.setArg(4, rtMatrix);
	renderKernel.setArg(6, rayCoef);
	
	cl::CommandQueue clQueue = cl::CommandQueue(clContext, device);

	uint8_t* cgBuf = manCTG::buf();
	uint32_t cgBufSize;

	uint8_t* gcMap;

	cl::Event cgWriEvent;
	cl::Event cgProEvent;
	cl::Event glAquEvent;
	cl::Event rtMatEvent;
	cl::Event glRenEvent;

	std::vector<cl::Event> cgProPrevents;
	std::vector<cl::Event> glRenPrevents;
	std::vector<cl::Event> glRelPrevents;

	uint32_t cgInsSyncAmount = 0;
	uint32_t cgInsAsyncAmount = 0;
	uint32_t cgInsSumAmount;
	
	cl_float3 norPos;

	camera::rad(10.0f);
	
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

	// Init light lookup table

	{
		clQueue.enqueueWriteBuffer(ldLookup, true, 0, 64, (void*)&Material::litDir);
		clQueue.finish();
	}

	// Init voxel buffer by inserting the root

	{
		manVox::init();

		cgBufSize = manCTG::wri(cgInsSyncAmount, cgInsAsyncAmount);
		cgInsSumAmount = cgInsSyncAmount + cgInsAsyncAmount;

		clQueue.enqueueWriteBuffer(cgBuffer, true, 0, cgBufSize, (void*)cgBuf, 0, &cgWriEvent);

		cgProKernel.setArg(2, cgInsSyncAmount);
		cgProKernel.setArg(3, cgInsAsyncAmount);
		cgProPrevents = { cgWriEvent };

		for (uint32_t i = 0; i < cgInsSumAmount; i += min(1024, cgInsSumAmount - i)) {
			clQueue.enqueueNDRangeKernel(cgProKernel, cl::NDRange(i), cl::NDRange(min(1024, cgInsSumAmount - i)), cl::NullRange, &cgProPrevents, &cgProEvent);
		}

		clQueue.finish();
	}

	testVoxelBufferData();

	while (!quit) {
		// Enqueue CPU to GPU instrcutions
		
		cgBufSize = manCTG::wri(cgInsSyncAmount, cgInsAsyncAmount);
		cgInsSumAmount = cgInsSyncAmount + cgInsAsyncAmount;
		
		if (cgBufSize > 0) {
			clQueue.enqueueWriteBuffer(cgBuffer, true, 0, cgBufSize, (void*)cgBuf, 0, &cgWriEvent);

			cgProKernel.setArg(2, cgInsSyncAmount);
			cgProKernel.setArg(3, cgInsAsyncAmount);
			cgProPrevents = {cgWriEvent};

			for (uint32_t i = 0; i < cgInsSumAmount; i += min(1024, cgInsSumAmount - i)) {
				clQueue.enqueueNDRangeKernel(cgProKernel, cl::NDRange(i), cl::NDRange(min(1024, cgInsSumAmount - i)), cl::NullRange, &cgProPrevents, &cgProEvent);
			}
		}

		clQueue.finish();

		glClear(GL_COLOR_BUFFER_BIT);
		glFinish();

		clQueue.enqueueAcquireGLObjects(&glMemory, 0, &glAquEvent);

		camera::mat(rot);

		clQueue.enqueueWriteBuffer(rtMatrix, true, 0, sizeof(cl_float) * 9, (void*)rot, 0, &rtMatEvent);

		glRenPrevents = { glAquEvent, rtMatEvent };
		glRenPrevents.clear();

		camera::pos(pos[0], pos[1], pos[2]);

		norPos.x = pos[0] * scale + 1.5f;
		norPos.y = pos[1] * scale + 1.5f;
		norPos.z = pos[2] * scale + 1.5f;

		renderKernel.setArg(5, norPos);
		
		for (uint16_t h = 0; h < height; h += min(32, height - h)) {
			for (uint16_t w = 0; w < width; w += min(32, width - w)) {
				clQueue.enqueueNDRangeKernel(renderKernel, cl::NDRange(w, h), cl::NDRange(min(32, width - w), min(32, height - h)), cl::NullRange, &glRenPrevents, &glRenEvent);

				glRenPrevents.push_back(std::move(glRenEvent));
			}
		}
		
		clQueue.enqueueReleaseGLObjects(&glMemory, &glRelPrevents);

		gcMap = (uint8_t*)clQueue.enqueueMapBuffer(gcBuffer, CL_TRUE, CL_MAP_READ, 0, 0x01 << 16);

		// TO DO: Read GPU to CPU communication

		clQueue.enqueueUnmapMemObject(gcBuffer, gcMap);

		clQueue.finish();

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glDrawBuffer(GL_BACK);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
		glReadBuffer(GL_COLOR_ATTACHMENT0);

		glBlitFramebuffer(0, 0, width, height, 0, 0, isFullscreen ? fullWidth : width, isFullscreen ? fullHeight : height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

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
	Material::matPbMl mats[VOXEL_DEPTH + 1] = { 0x00 };

	mats[VOXEL_DEPTH] = Material::parR_to_mat(0) | Material::parG_to_mat(63) | Material::parB_to_mat(0) | Material::beta_to_mat(63) |
						Material::medR_to_mat(0) | Material::medG_to_mat(0) | Material::medB_to_mat(0) | Material::emit_to_mat(1) | 0x0000000000FFFFFF;

	for (int i = (VOXEL_DEPTH - 1); i >= 0; --i) {
		mats[i] = Material::mat_avg_col(mats[i + 1], 0x000000FFFFFFFFFF, 0x000000FFFFFFFFFF, 0x000000FFFFFFFFFF,
			0x000000FFFFFFFFFF, 0x000000FFFFFFFFFF, 0x000000FFFFFFFFFF, 0x000000FFFFFFFFFF) | 0x0000000000FFFFFF;
	}

	std::unique_ptr<INS_CTG> t;

	uint8_t d[5] = { (mats[0] >> 56) & 0xFF, (mats[0] >> 48) & 0xFF, (mats[0] >> 40) & 0xFF, (mats[0] >> 32) & 0xFF, (mats[0] >> 24) & 0xFF };
	std::unique_ptr<uint8_t[]> c(new uint8_t[5]());
	memcpy(c.get(), &d, 5 * 4);
	t.reset(new INS_CTG_COL(0, std::move(c)));
	manCTG::eqA(std::move(t));

	d[0] = (mats[0] >> 16) & 0xFF; d[1] = (mats[0] >> 8) & 0xFF; d[2] = mats[0] & 0xFF;
	std::unique_ptr<uint8_t[]> l(new uint8_t[3]());
	memcpy(l.get(), &d, 3 * 4);
	t.reset(new INS_CTG_LIT(0, std::move(l)));
	manCTG::eqA(std::move(t));

	uint32_t par = 0x00;
	uint32_t idx = 0x08;

	for (int i = 1; i <= VOXEL_DEPTH; ++i) {
		idx = 8 * i;
		
		t.reset(new INS_CTG_EXP(par, idx));
		manCTG::eqS(std::move(t));

		par = idx + 7;
	}

	uint32_t o[8] = { 0x00 };
	std::unique_ptr<uint32_t[]> v(new uint32_t[8]());

	par = 0x00;

	o[0] = 0xF0000000;
	o[1] = 16;
	o[2] = mats[1] >> 32;
	o[3] = mats[1] & 0xFFFFFFFF;

	v.reset(new uint32_t[8]());
	memcpy(v.get(), &o, 8 * 4);
	t.reset(new INS_CTG_ADD(par, std::move(v)));
	manCTG::eqS(std::move(t));

	par = 15;

	for (int i = 2; i <= VOXEL_DEPTH; ++i) {
		o[0] = 0x80000000 | ((par & 0x0F) << 24);
		o[1] = 8 * (i + 1);
		o[2] = mats[i] >> 32;
		o[3] = mats[i] & 0xFFFFFFFF;
		
		v.reset(new uint32_t[8]());
		memcpy(v.get(), &o, 8 * 4);
		t.reset(new INS_CTG_ADD(par, std::move(v)));
		manCTG::eqS(std::move(t));

		par = 8 * i;
	}
}