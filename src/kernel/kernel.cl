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

// Voxel structure on the GPU
// 32-bit structure: 1-active, 3-index, 4-parent, 8-children, (16-unused)
// 32-bit children: 32-pointer
// 32 bit colour: 8-pred, 8-pgreen, 8-pblue, 8-beta       \ "A physically Based Colour Model"
// 32 bit colour: 8-mred, 8-mgreen, 8-mblue, (8-unused)   /  Robert J Oddy, Philip J Willis

// Voxel structure on the CPU
// 32-bit structure: 1-active, 3-index, 4-parent, 8-children, (16-unused)
// 32-bit children: 32-pointer
// 32 bit unused: (32-unused)
// 32 bit unused: (32-unused)

// Voxel structure on CPU and GPU are synchornized

// Linked list for loading spaces on the CPU

// Pointers are absolute and refer to the block of voxel data

// Only reserve ~1GB of VRAM -> not entirety of 32-bit pointers is needed

// Unloading still UNSOLVED -> rendered bit?

// Loading of voxels in Morton order/Z ordering (interwoven coordinate bits: zyxzyx) -> voxel level?