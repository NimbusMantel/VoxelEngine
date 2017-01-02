#include "asset_utils.h"
#include "platform_asset_utils.h"
#include "shader.h"
//#include "texture.h"
#include <assert.h>
#include <stdlib.h>

GLuint build_program_from_assets(const char* vertex_shader_path, const char* fragment_shader_path) {
	assert(vertex_shader_path != NULL);
	assert(fragment_shader_path != NULL);

	const FileData vertex_shader_source = get_asset_data(vertex_shader_path);
	const FileData fragment_shader_source = get_asset_data(fragment_shader_path);

	const GLchar* fr_src = vertex_shader_source.data;

	const GLuint program_object_id = build_program(vertex_shader_source.data, vertex_shader_source.data_length,fragment_shader_source.data, fragment_shader_source.data_length);

	release_asset_data(&vertex_shader_source);
	release_asset_data(&fragment_shader_source);

	return program_object_id;
}