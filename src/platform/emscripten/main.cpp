#include <stdlib.h>
#include <stdio.h>
#include <GL/glfw.h>
#include <emscripten/emscripten.h>
#include "game.h"

#include <chrono>

int init_gl(void);
void do_frame();
void shutdown_gl();
void handle_input();

static int is_dragging;

const int width = 640, height = 360;

typedef std::chrono::high_resolution_clock Time;

static std::chrono::time_point<Time> currFrame;
static std::chrono::time_point<Time> lastFrame;
static std::chrono::duration<double> deltaTime;

int main(void)
{
	lastFrame = Time::now();
	
	if (init_gl() == GL_TRUE) {
		on_surface_created();
		on_surface_changed();
		emscripten_set_main_loop(do_frame, 0, 1);
	}

	shutdown_gl();

	return 0;
}

int init_gl()
{
	if (glfwInit() != GL_TRUE) {
		printf("glfwInit() failed\n");
		return GL_FALSE;
	}

	if (glfwOpenWindow(width, height, 8, 8, 8, 8, 16, 0, GLFW_WINDOW) != GL_TRUE) {
		printf("glfwOpenWindow() failed\n");
		return GL_FALSE;
	}

	return GL_TRUE;
}

void do_frame()
{
	currFrame = Time::now();
	deltaTime = currFrame - lastFrame;
	
	handle_input();
	on_draw_frame(deltaTime.count());
	glfwSwapBuffers();

	lastFrame = currFrame;
}

void shutdown_gl()
{
	glfwTerminate();
}

void handle_input() {
	glfwPollEvents();


	const int left_mouse_button_state = glfwGetMouseButton(GLFW_MOUSE_BUTTON_1);
	if (left_mouse_button_state == GLFW_PRESS) {
		int x_pos, y_pos;

		glfwGetMousePos(&x_pos, &y_pos);

		const float normalized_x = ((float)x_pos / (float)width) * 2.f - 1.f;
		const float normalized_y = -(((float)y_pos / (float)height) * 2.f - 1.f);

		if (is_dragging == 0) {
			is_dragging = 1;
			on_touch_press(normalized_x, normalized_y);
		}
		else {
			on_touch_drag(normalized_x, normalized_y);
		}
	}
	else {
		is_dragging = 0;
	}
}