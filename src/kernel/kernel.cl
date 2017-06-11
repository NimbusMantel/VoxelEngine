typedef unsigned int uint32_t;

#if OpenCLDebug
__kernel void test(__write_only image2d_t rbo, __global __read_write uint32_t* vxBuffer, __global __read_only char* cgBuffer, __global __read_write char* gcBuffer, __global char* debug) {
	//debug[0] = cgBuffer[0];
#else
__kernel void test(__write_only image2d_t rbo, __global __read_write uint32_t* vxBuffer, __global __read_only char* cgBuffer, __global __read_write char* gcBuffer) {
#endif
	int width = get_image_width(rbo);
	int height = get_image_height(rbo);

	for (int h = 0; h < height; ++h) {
		for (int w = 0; w < width; ++w) {
			write_imagef(rbo, (int2)(w, h), (float4)(1, 1, 0, 1));
		}
	}
}