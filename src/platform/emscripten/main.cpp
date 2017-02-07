#include <stdio.h>
#include <functional>
#include <assert.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

#include <GLES2/gl2.h>

#include <emscripten/emscripten.h>

#include "game.h"

#include "platform_log.h"

void update();
void input();

static const int width = 640, height = 360;
static const GLsizei texSize = [](GLsizei v) { v--; v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v++; return v; }((width > height) ? width : height);

static SDL_Window* window;
static SDL_GLContext context;

static GLuint screen;
static GLuint buffer;
static GLuint shader;

static GLint a_Position;
static GLint a_Coordinate;
static GLint u_Texture;

static Uint32 pixels[width * height];
static Uint8 mask[width * height];

Uint32 currFrame;
Uint32 prevFrame;

bool isDragging = false;

int main(void) {
	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow(NULL, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_OPENGL);
	context = SDL_GL_CreateContext(window);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glGenTextures(1, &screen);
	glBindTexture(GL_TEXTURE_2D, screen);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texSize, texSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glBindTexture(GL_TEXTURE_2D, 0);

	float rect[] = { -1.0f, -1.0f, 0.0f, 0.0f,
					 -1.0f,  1.0f, 0.0f, height / (float)texSize,
					 1.0f, -1.0f, width / (float)texSize, 0.0f,
					 1.0f,  1.0f, width / (float)texSize, height / (float)texSize };

	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rect), (GLvoid*)rect, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	const GLchar* vertexSource[] = { 
									 "attribute vec4 a_Position;"
									 "attribute vec2 a_Coordinate;"
									 "varying vec2 v_Coordinate;"
									 "void main() {"
									 "	 v_Coordinate = a_Coordinate;"
									 "	 gl_Position = a_Position;"
									 "}"
								   };

	const GLchar* fragmeSource[] = {
									 "precision mediump float;"
									 "uniform sampler2D u_Texture;"
									 "varying vec2 v_Coordinate;"
									 "float hueTOrgb(float hue) {"
									 "	 if (hue < 0.0) hue += 1.0;"
									 "	 else if (hue >= 1.0) hue -= 1.0;"
									 "	 if ((6.0 * hue) < 1.0) return hue * 6.0;"
									 "	 else if ((2.0 * hue) < 1.0) return 1.0;"
									 "	 else if ((3.0 * hue) < 2.0) return (2.0/3.0 - hue) * 6.0;"
									 "	 else return 0.0;"
									 "}"
									 "void main() {"
									 "	 vec4 hwba = texture2D(u_Texture, v_Coordinate);"
									 "	 hwba = vec4(hwba.w, hwba.z, hwba.y, hwba.x);"
									 "	 if (hwba.w == 0.0) {"
									 "		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);"
									 "		return;"
									 "	 }"
									 "   hwba.x = ((hwba.x * 255.0 - ((hwba.x >= 0.5) ? 128.0 : 0.0)) * 4.0 + ((hwba.y >= 0.75) ? 3.0 : ((hwba.y >= 0.5) ? 2.0 : ((hwba.y >= 0.25) ? 1.0 : 0.0)))) / 360.0;"
									 "   hwba.y = ((hwba.y * 255.0 - ((hwba.y >= 0.75) ? 192.0 : ((hwba.y >= 0.5) ? 128.0 : ((hwba.y >= 0.25) ? 64.0 : 0.0)))) * 2.0 + ((hwba.z >= 0.5) ? 1.0 : 0.0)) / 127.0;"
									 "	 hwba.z = (hwba.z * 255.0 - ((hwba.z >= 0.5) ? 128.0 : 0.0)) / 127.0;"
									 "	 if (hwba.w < 1.0) {"
									 "		hwba.y *= (hwba.w / (hwba.w + 1.0));"
									 "		hwba.z += (1.0 - hwba.z) * (1.0 / (hwba.w + 1.0));"
									 "	 }"
									 "	 vec4 rgba = vec4(hueTOrgb(hwba.x + 1.0/3.0), hueTOrgb(hwba.x), hueTOrgb(hwba.x - 1.0/3.0), 1.0);"
									 "	 rgba.x = (rgba.x * (1.0 - hwba.y - hwba.z) + hwba.y);"
									 "	 rgba.y = (rgba.y * (1.0 - hwba.y - hwba.z) + hwba.y);"
									 "	 rgba.z = (rgba.z * (1.0 - hwba.y - hwba.z) + hwba.y);"
									 "	 gl_FragColor = rgba;"
									 "}"
								   };

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, vertexSource, NULL);
	glCompileShader(vertexShader);
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, fragmeSource, NULL);
	glCompileShader(fragmentShader);
	shader = glCreateProgram();
	glAttachShader(shader, vertexShader);
	glAttachShader(shader, fragmentShader);
	glLinkProgram(shader);

	a_Position = glGetAttribLocation(shader, "a_Position");
	a_Coordinate = glGetAttribLocation(shader, "a_Coordinate");
	u_Texture = glGetUniformLocation(shader, "u_Texture");

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	prevFrame = SDL_GetTicks();

	on_init(width, height, pixels, mask);

	emscripten_set_main_loop(update, 0, 1);

	SDL_DestroyWindow(window);
	window = NULL;

	SDL_Quit();

	return 0;
}

void update() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	currFrame = SDL_GetTicks();

	input();
	on_update((currFrame - prevFrame) / 1000.0f);

	prevFrame = currFrame;

	glBindTexture(GL_TEXTURE_2D, screen);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glBindTexture(GL_TEXTURE_2D, 0);

	glUseProgram(shader);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, screen);
	glUniform1i(u_Texture, 0);

	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glVertexAttribPointer(a_Position, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GL_FLOAT), (void*)(0));
	glVertexAttribPointer(a_Coordinate, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GL_FLOAT), (void*)(2 * sizeof(GL_FLOAT)));
	glEnableVertexAttribArray(a_Position);
	glEnableVertexAttribArray(a_Coordinate);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	SDL_GL_SwapWindow(window);
}

void input() {
	SDL_Event event;

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_MOUSEBUTTONDOWN: if (event.button.button == SDL_BUTTON_LEFT) { isDragging = true;  on_touch_down(event.button.x, height - event.button.y - 1); } break;
		case SDL_MOUSEBUTTONUP: if (event.button.button == SDL_BUTTON_LEFT) { isDragging = false; on_touch_up(event.button.x, height - event.button.y - 1); } break;
		case SDL_MOUSEMOTION: if (isDragging) { on_touch_move(event.motion.x, height - event.motion.y - 1, event.motion.xrel, -event.motion.yrel); } break;
		case SDL_MOUSEWHEEL: on_mouse_scroll(event.wheel.x, event.wheel.y); break;
		}
	}
}