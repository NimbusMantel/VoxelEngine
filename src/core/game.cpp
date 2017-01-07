#include "game.h"

#include "buffer.h"
#include "platform_gl.h"
#include "platform_log.h"
#include "asset_utils.h"
#include "voxel.h"

//#include <stdlib.h>

static const GLsizei winWidth = 16;
static const GLsizei winHeight = 9;

static const GLsizei texSize = [](GLsizei v){ v--; v |= v >> 1; v |= v >> 2; v |= v >> 4; v |= v >> 8; v++; return v; }((winWidth > winHeight) ? winWidth : winHeight);

static GLubyte pixels[winWidth * winHeight];
static GLubyte colours[256 * 3] = {	0, 0, 0, 128, 0, 0, 0, 128, 0, 128, 128, 0, 0, 0, 128, 128, 0, 128, 0, 128, 128, 192, 192, 192, 128, 128, 128, 255,
									0, 0, 0, 255, 0, 255, 255, 0, 0, 0, 255, 255, 0, 255, 0, 255, 255, 255, 255, 255, 0, 0, 0, 0, 0, 95, 0, 0, 135, 0,
									0, 175, 0, 0, 215, 0, 0, 255, 0, 95, 0, 0, 95, 95, 0, 95, 135, 0, 95, 175, 0, 95, 215, 0, 95, 255, 0, 135, 0, 0, 135,
									95, 0, 135, 135, 0, 135, 175, 0, 135, 215, 0, 135, 255, 0, 175, 0, 0, 175, 95, 0, 175, 135, 0, 175, 175, 0, 175, 215,
									0, 175, 255, 0, 215, 0, 0, 215, 95, 0, 215, 135, 0, 215, 175, 0, 215, 215, 0, 215, 255, 0, 255, 0, 0, 255, 95, 0, 255,
									135, 0, 255, 175, 0, 255, 215, 0, 255, 255, 95, 0, 0, 95, 0, 95, 95, 0, 135, 95, 0, 175, 95, 0, 215, 95, 0, 255, 95,
									95, 0, 95, 95, 95, 95, 95, 135, 95, 95, 175, 95, 95, 215, 95, 95, 255, 95, 135, 0, 95, 135, 95, 95, 135, 135, 95, 135,
									175, 95, 135, 215, 95, 135, 255, 95, 175, 0, 95, 175, 95, 95, 175, 135, 95, 175, 175, 95, 175, 215, 95, 175, 255, 95,
									215, 0, 95, 215, 95, 95, 215, 135, 95, 215, 175, 95, 215, 215, 95, 215, 255, 95, 255, 0, 95, 255, 95, 95, 255, 135, 95,
									255, 175, 95, 255, 215, 95, 255, 255, 135, 0, 0, 135, 0, 95, 135, 0, 135, 135, 0, 175, 135, 0, 215, 135, 0, 255, 135,
									95, 0, 135, 95, 95, 135, 95, 135, 135, 95, 175, 135, 95, 215, 135, 95, 255, 135, 135, 0, 135, 135, 95, 135, 135, 135,
									135, 135, 175, 135, 135, 215, 135, 135, 255, 135, 175, 0, 135, 175, 95, 135, 175, 135, 135, 175, 175, 135, 175, 215,
									135, 175, 255, 135, 215, 0, 135, 215, 95, 135, 215, 135, 135, 215, 175, 135, 215, 215, 135, 215, 255, 135, 255, 0, 135,
									255, 95, 135, 255, 135, 135, 255, 175, 135, 255, 215, 135, 255, 255, 175, 0, 0, 175, 0, 95, 175, 0, 135, 175, 0, 175,
									175, 0, 215, 175, 0, 255, 175, 95, 0, 175, 95, 95, 175, 95, 135, 175, 95, 175, 175, 95, 215, 175, 95, 255, 175, 135, 0,
									175, 135, 95, 175, 135, 135, 175, 135, 175, 175, 135, 215, 175, 135, 255, 175, 175, 0, 175, 175, 95, 175, 175, 135, 175,
									175, 175, 175, 175, 215, 175, 175, 255, 175, 215, 0, 175, 215, 95, 175, 215, 135, 175, 215, 175, 175, 215, 215, 175,
									215, 255, 175, 255, 0, 175, 255, 95, 175, 255, 135, 175, 255, 175, 175, 255, 215, 175, 255, 255, 215, 0, 0, 215, 0, 95,
									215, 0, 135, 215, 0, 175, 215, 0, 215, 215, 0, 255, 215, 95, 0, 215, 95, 95, 215, 95, 135, 215, 95, 175, 215, 95, 215,
									215, 95, 255, 215, 135, 0, 215, 135, 95, 215, 135, 135, 215, 135, 175, 215, 135, 215, 215, 135, 255, 223, 175, 0, 223,
									175, 95, 223, 175, 135, 223, 175, 175, 223, 175, 223, 223, 175, 255, 223, 223, 0, 223, 223, 95, 223, 223, 135, 223, 223,
									175, 223, 223, 223, 223, 223, 255, 223, 255, 0, 223, 255, 95, 223, 255, 135, 223, 255, 175, 223, 255, 223, 223, 255, 255,
									255, 0, 0, 255, 0, 95, 255, 0, 135, 255, 0, 175, 255, 0, 223, 255, 0, 255, 255, 95, 0, 255, 95, 95, 255, 95, 135, 255,
									95, 175, 255, 95, 223, 255, 95, 255, 255, 135, 0, 255, 135, 95, 255, 135, 135, 255, 135, 175, 255, 135, 223, 255, 135,
									255, 255, 175, 0, 255, 175, 95, 255, 175, 135, 255, 175, 175, 255, 175, 223, 255, 175, 255, 255, 223, 0, 255, 223, 95,
									255, 223, 135, 255, 223, 175, 255, 223, 223, 255, 223, 255, 255, 255, 0, 255, 255, 95, 255, 255, 135, 255, 255, 175, 255,
									255, 223, 255, 255, 255, 8, 8, 8, 18, 18, 18, 28, 28, 28, 38, 38, 38, 48, 48, 48, 58, 58, 58, 68, 68, 68, 78, 78, 78, 88,
									88, 88, 98, 98, 98, 108, 108, 108, 118, 118, 118, 128, 128, 128, 138, 138, 138, 148, 148, 148, 158, 158, 158, 168, 168,
									168, 178, 178, 178, 188, 188, 188, 198, 198, 198, 208, 208, 208, 218, 218, 218, 228, 228, 228, 238, 238, 238 };

static GLuint buffer;
static GLuint texture;
static GLuint program;
static GLuint palette;

static GLint a_position_location;
static GLint a_texture_coordinates_location;
static GLint u_texture_unit_location;
static GLint u_texture_palette;

static const float rect[] = { -1.0f, -1.0f, 0.0f, 0.0f,
							  -1.0f,  1.0f, 0.0f, winHeight / (float)texSize,
							  1.0f, -1.0f, winWidth / (float)texSize, 0.0f,
							  1.0f,  1.0f, winWidth / (float)texSize, winHeight / (float)texSize };

static VoxelBuffer voxels = VoxelBuffer();

void testVoxels() {
	// Log the root voxel and its header

	DEBUG_LOG_RAW("VoxelBuffer Test", "%s", "Log the root voxel and its header");

	voxels.logVoxel(0); // V 0
	voxels.logVoxel(1); // H 0:0

	// Add two voxels to the root

	DEBUG_LOG_RAW("VoxelBuffer Test", "%s", "Add two voxels to the root");

	voxels.setVoxel(0, 0, VoxelBuffer::constructVoxel(255)); // V 0-0
	voxels.setVoxel(0, 7, VoxelBuffer::constructVoxel(255)); // V 0-7

	voxels.logVoxel(0); // V 0
	voxels.logVoxel(2); // V 0-0
	voxels.logVoxel(9); // V 0-7
	voxels.logVoxel(10); // H 0-0:0-7

	// Add two voxels to (2) with one having a child

	DEBUG_LOG_RAW("VoxelBuffer Test", "%s", "Add two voxels to (2) with one having a child");

	voxels.setVoxel(2, 0, VoxelBuffer::constructVoxel(170)); // V 0-0-0
	voxels.setVoxel(2, 1, VoxelBuffer::constructVoxel(255)); // V 0-0-1
	voxels.setVoxel(11, 0, VoxelBuffer::constructVoxel(0)); // V 0-0-0-0

	voxels.logVoxel(2); // V 0-0
	voxels.logVoxel(9); // V 0-7
	voxels.logVoxel(10); // H 0-0:0-7
	voxels.logVoxel(11); // V 0-0-0
	voxels.logVoxel(12); // V 0-0-1
	voxels.logVoxel(19); // H 0-0-0:0-0-7
	voxels.logVoxel(20); // V 0-0-0-0
	voxels.logVoxel(28); // H 0-0-0-0:0-0-0-7

	// Move the first child of (2) to the first child of (9)

	DEBUG_LOG_RAW("VoxelBuffer Test", "%s", "Move the first child of (2) to the first child of (9)");

	voxels.movVoxel(2, 0, 9, 0); // V 0-0-0 -> V 0-7-0

	voxels.logVoxel(2); // V 0-0
	voxels.logVoxel(9); // V 0-7
	voxels.logVoxel(10); // H 0-0:0-7
	voxels.logVoxel(11); // Empty
	voxels.logVoxel(12); // V 0-0-1
	voxels.logVoxel(19); // H 0-0-0:0-0-7
	voxels.logVoxel(29); // V 0-7-0
	voxels.logVoxel(37); // H 0-7-0:0-7-7
	voxels.logVoxel(20); // V 0-7-0-0
	voxels.logVoxel(28); // H 0-7-0-0:0-7-0-7

	// Delete the remaining child of (2)

	DEBUG_LOG_RAW("VoxelBuffer Test", "%s", "Delete the remaining child of (2)");

	voxels.delVoxel(2, 1); // V 0-0-1

	voxels.logVoxel(0); // V 0
	voxels.logVoxel(2); // Empty
	voxels.logVoxel(11); // Empty
	voxels.logVoxel(12); // Empty
	voxels.logVoxel(19); // Empty
}

void on_surface_created() {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	
	//testVoxels();
}

void on_surface_changed() {
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, texSize, texSize, 0, GL_ALPHA, GL_UNSIGNED_BYTE, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &palette);
	glBindTexture(GL_TEXTURE_2D, palette);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, colours);
	glBindTexture(GL_TEXTURE_2D, 0);

	buffer = create_vbo(sizeof(rect), rect, GL_STATIC_DRAW);

	program = build_program_from_assets("shader.vsh", "shader.fsh");

	a_position_location = glGetAttribLocation(program, "a_Position");
	a_texture_coordinates_location = glGetAttribLocation(program, "a_TextureCoordinates");
	u_texture_unit_location = glGetUniformLocation(program, "u_TextureUnit");
	u_texture_palette = glGetUniformLocation(program, "u_TexturePalette");
}

void update() {
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, winWidth, winHeight, GL_ALPHA, GL_UNSIGNED_BYTE, pixels);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void on_draw_frame() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	voxels.renderFrame(winWidth, winHeight, pixels);
	update();

	glUseProgram(program);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(u_texture_unit_location, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, palette);
	glUniform1i(u_texture_palette, 1);

	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glVertexAttribPointer(a_position_location, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GL_FLOAT), BUFFER_OFFSET(0));
	glVertexAttribPointer(a_texture_coordinates_location, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GL_FLOAT), BUFFER_OFFSET(2 * sizeof(GL_FLOAT)));
	glEnableVertexAttribArray(a_position_location);
	glEnableVertexAttribArray(a_texture_coordinates_location);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}