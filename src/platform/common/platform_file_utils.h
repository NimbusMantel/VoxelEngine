#pragma once

struct FileData {
	const long data_length;
	const char* data;
	const void* file_handle;
};

FileData get_file_data(const char* path);
void release_file_data(const FileData* file_data);