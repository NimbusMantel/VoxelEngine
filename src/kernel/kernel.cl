#if OpenCLDebug
__kernel void test(__write_only image2d_t rbo, __global char* debug) {
#else
__kernel void test(__write_only image2d_t rbo) {
#endif
	int width = get_image_width(rbo);
	int height = get_image_height(rbo);

	for (int h = 0; h < height; ++h) {
		for (int w = 0; w < width; ++w) {
			write_imagef(rbo, (int2)(w, h), (float4)(1, 1, 0, 1));
		}
	}
}