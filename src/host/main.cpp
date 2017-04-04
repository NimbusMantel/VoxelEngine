#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <CL/cl.hpp>

#include <iostream>

#include <vector>
#include <fstream>

static SDL_Window* window;
static SDL_GLContext context;

static const int width = 640, height = 360;

bool setupOpenGL();
void printSDLError(int line);
void run();
void cleanup();

bool init() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "Error: Failed to init SDL" << std::endl;

		return false;
	}

	window = SDL_CreateWindow("Voxel Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);

	if (!window) {
		std::cout << "Error: Unable to create window" << std::endl;

		printSDLError(__LINE__);

		return false;
	}

	context = SDL_GL_CreateContext(window);

	setupOpenGL();

	SDL_GL_SetSwapInterval(1);

	return true;
}

bool setupOpenGL()
{
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	return true;
}

int main(int argc, char* argv[]) {
	if (!init()) {
		return -1;
	}
		
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	SDL_GL_SwapWindow(window);

	run();

	cleanup();

	return 0;
}

void run() {
	bool loop = true;

	while (loop) {
		SDL_Event event;

		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT)
				loop = false;

			if (event.type == SDL_KEYDOWN) {
				switch (event.key.keysym.sym) {
				case SDLK_ESCAPE:
					loop = false;
					break;
				case SDLK_r:
					glClearColor(1.0, 0.0, 0.0, 1.0);
					glClear(GL_COLOR_BUFFER_BIT);
					SDL_GL_SwapWindow(window);
					break;
				case SDLK_g:
					glClearColor(0.0, 1.0, 0.0, 1.0);
					glClear(GL_COLOR_BUFFER_BIT);
					SDL_GL_SwapWindow(window);
					break;
				case SDLK_b:
					glClearColor(0.0, 0.0, 1.0, 1.0);
					glClear(GL_COLOR_BUFFER_BIT);
					SDL_GL_SwapWindow(window);
					break;
				default:
					break;
				}
			}
		}
	}
}

void cleanup() {
	SDL_GL_DeleteContext(context);

	SDL_DestroyWindow(window);

	SDL_Quit();
}

void printSDLError(int line = -1)
{
	std::string error = SDL_GetError();

	if (error != "") {
		std::cout << "SLD Error : " << error << std::endl;

		if (line != -1) {
			std::cout << std::endl << "Line : " << line << std::endl;
		}

		SDL_ClearError();
	}
}

/*

std::vector<cl::Platform> platforms;
cl::Platform::get(&platforms);

auto platform = platforms.front();

std::vector<cl::Device> devices;
platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

auto device = devices.front();

std::ifstream helloWorldFile("src/kernel/kernel.cl");
std::string src(std::istreambuf_iterator<char>(helloWorldFile), (std::istreambuf_iterator<char>()));

cl::Program::Sources sources(1, std::make_pair(src.c_str(), src.length() + 1));

cl::Context context(device);
cl::Program program(context, sources);

auto err = program.build("-cl-std=CL1.2");

char buff[16];
cl::Buffer memBuf(context, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, sizeof(buff));
cl::Kernel kernel(program, "HelloWorld", &err);
kernel.setArg(0, memBuf);

cl::CommandQueue queue(context, device);
queue.enqueueTask(kernel);
queue.enqueueReadBuffer(memBuf, CL_TRUE, 0, sizeof(buff), buff);

std::cout << buff;

*/