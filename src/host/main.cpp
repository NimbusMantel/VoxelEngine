#include <SDL2/SDL.h>
#include <GL/glew.h>

#include <CL/cl.hpp>

#include <map>
#include <vector>
#include <fstream>

#include <iostream>

#include <Windows.h>

#include "voxel/voxel.hpp"

#define OpenCLDebug 0

int main(int argc, char* argv[]) {
	// Voxel buffer binary tree testing:
	//            TO DO

	if (SDL_Init(SDL_INIT_VIDEO) < 0) return -1;

	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	const int width = 640, height = 360;

	char titat[] = "Voxel Engine (%d fps)";
	char title[32];

	SDL_Window* window = SDL_CreateWindow("Voxel Engine (0 fps)", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL);
	
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
	 
	cl_context_properties props[] =
	{
		CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
		CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
		0
	};

	cl::Context clContext = cl::Context(device, props);
	
	cl::BufferRenderGL clBuffer = cl::BufferRenderGL(clContext, CL_MEM_WRITE_ONLY, rbo, NULL);
	glFinish();
	std::vector<cl::Memory> clMemory = { clBuffer };

#if OpenCLDebug
	char buff[256];
	cl::Buffer debBuf(clContext, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, sizeof(buff));
#endif

	std::ifstream strm("src/kernel/kernel.cl");
	std::string src(std::istreambuf_iterator<char>(strm), (std::istreambuf_iterator<char>()));

	cl::Program::Sources sources(1, std::make_pair(src.c_str(), src.length() + 1));

	cl::Program clProgram(clContext, sources);

	char clBuild[] = "-cl-std=CL1.2 -D OpenCLDebug=0";

#if OpenCLDebug
	clBuild[29] = '1';
#endif

	clProgram.build(clBuild);

	cl::Kernel clKernel(clProgram, "test", NULL);
	clKernel.setArg(0, clBuffer);
#if OpenCLDebug
	clKernel.setArg(1, debBuf);
#endif

	cl::CommandQueue clQueue = cl::CommandQueue(clContext, device);

	int currentFrame, previousFrame = SDL_GetTicks(), fps;

	bool quit = false;

	SDL_Event event;

	while (!quit) {
		glClear(GL_COLOR_BUFFER_BIT);

		glFinish();

		clQueue.enqueueAcquireGLObjects(&clMemory, NULL, NULL);
		clQueue.enqueueTask(clKernel);
#if OpenCLDebug
		clQueue.enqueueReadBuffer(debBuf, CL_TRUE, 0, sizeof(buff), buff);
		std::cout << buff;
#endif
		clQueue.enqueueReleaseGLObjects(&clMemory, NULL, NULL);

		clQueue.finish();

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glDrawBuffer(GL_BACK);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
		glReadBuffer(GL_COLOR_ATTACHMENT0);

		glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		SDL_GL_SwapWindow(window);

		currentFrame = SDL_GetTicks();
		fps = (60000 / (currentFrame - previousFrame));
		sprintf(title, titat, fps);
		SDL_SetWindowTitle(window, title);
		previousFrame = currentFrame;

		while (SDL_PollEvent(&event)) {
			if ((event.type == SDL_QUIT) || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
				quit = true;
			}
		}
	}

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}