#include "voxel.h"

#include "colour.h"

#include "platform_log.h"

#include <stdlib.h>
#include <assert.h>
#include <bitset>
#include <math.h>

// Voxel: 15 Bit first child - 1 Bit far pointer - 8 Bit children mask - 8 Bit unused - 1 Bit colour changed - 9 Bit hue - 7 Bit white - 7 Bit black - 8 Bit alpha
// Parent: 16 Bit 0x0001 - 15 Bit parent - 1 Bit far pointer
// Pointer: 32 Bit pointer

// close pointers are always relative, far pointers always absolute

VoxelBuffer::VoxelBuffer() {
	buffer = (uint32_t*)malloc(sizeof(uint32_t) * VOXEL_BUFFER_LENGTH);
	size = VOXEL_BUFFER_LENGTH;
	
	buffer[0] = 0xFFFF0000; // empty root voxel
	buffer[1] = 0x00000000; // empty root colour
	buffer[2] = 0x0001FFFC; // parent header pointing to the root voxel

	spots[3] = VOXEL_BUFFER_LENGTH - 3;
}

VoxelBuffer::~VoxelBuffer() {
	free(buffer);
}

uint64_t VoxelBuffer::constructVoxel(uint32_t colour) {
	return ((uint64_t)0xFFFF000000000000 | rgbaTOhwba(colour));
}

uint32_t VoxelBuffer::allocBufferSpace(uint32_t length) {
	std::map<uint32_t, uint32_t>::iterator it;
	
	for (it = spots.begin(); it != spots.end(); ++it) {
		if (it->second >= length) { // Find a spot with enough length
			break;
		}
	}

	assert("Voxel buffer size increase error" && it != spots.end());

	spots[it->first + length] = it->second - length;

	spots.erase(it);

	return it->first;
}

void VoxelBuffer::freeBufferSpace(uint32_t index, uint32_t length) {
	for (uint32_t i = index; i < (index + length); ++i) {
		buffer[i] = 0x00000000;
	}
	
	spots[index] = length;

	std::map<uint32_t, uint32_t>::iterator it = spots.find(index);

	it++;

	if (it != spots.end()) {
		if ((index + length) == it->first) { // Check if the new spot is a pre-extension of an existing one
			spots[index] += it->second;

			spots.erase(it);
		}
	}

	it--;

	if (it != spots.begin()) {
		it--;

		if ((it->first + it->second) == index) { // Check if the new spot is a post-extension of an existing one
			it->second += length;

			spots.erase(++it);
		}
	}
}

void VoxelBuffer::clearVoxel(uint32_t index, bool clearParent, bool clearChildren) {
	uint32_t Voxel = buffer[index];
	
	if (!Voxel || (Voxel >> 17) == 0) {
		return;
	}
	
	uint32_t Pointer;
	uint8_t i;

	if (clearChildren && ((Voxel & 0x0000FF00) >> 8) != 0) { // Voxel has children that are to be cleared
		Pointer = index + (int16_t)((Voxel & 0x7FFE0000) >> 17 | (Voxel & 0x80000000) >> 16 | (Voxel & 0x80000000) >> 17); // First child pointer
		
		if ((Voxel & 0x00010000) == 1) {
			Voxel = buffer[Pointer];

			freeBufferSpace(Pointer, 1); // Free the pointer to the first child

			Pointer = Voxel;

			Voxel = buffer[index];
		}
		
		for (i = 0; i < 8; ++i) {
			clearVoxel(Pointer, clearParent, true); // Clear the voxel's children

			Pointer += 2;
		}

		if ((buffer[Pointer] & 0x00000001) == 1) {
			freeBufferSpace(index + (int16_t)((buffer[Pointer] & 0x00007FFE) >> 1 | (buffer[Pointer] & 0x00008000) | (buffer[Pointer] & 0x00008000) >> 1), 1); // Remove the parent header's pointer to the voxel
		}

		freeBufferSpace(Pointer - 16, 17); // Free the buffer of the children
	}

	if (index > 1) { // Voxel isn't the root
		buffer[index] = buffer[index + 1] = 0x00000000;

		i = 0;

		while ((buffer[index + i] >> 16) != 1) {
			i += 2; // Get the distance to the parent header
		}

		Pointer = index + i + (int16_t)((buffer[index + i] & 0x00007FFE) >> 1 | (buffer[index + i] & 0x00008000) | (buffer[index + i] & 0x00008000) >> 1); // Pointer to the voxel's parent
		
		if (buffer[index + i] & 0x00000001) {
			Pointer = buffer[Pointer];
		}
		
		if (clearParent && (((buffer[Pointer] & (~(0x01 << (8 + (i >> 1) - 1)))) & 0x0000FF00) >> 8) == 0) {
			clearVoxel(Pointer, true, clearChildren); // Clear the voxel's parent
		}
		else {
			buffer[Pointer] &= ~(0x01 << (8 + (i >> 1) - 1)); // Update the parent's children mask
		}
	}
	else {
		buffer[index] = ((Voxel & 0x000000FF) | 0xFFFF0000); // Clear the root's children mask and reset it's first child pointer
	}
}

bool VoxelBuffer::setVoxel(uint32_t parent, uint8_t position, uint64_t voxel) {
	if ((voxel >> 48) == 1 || position > 7 || (voxel & 0x0000FF0000000000) > 0) {
		return false;
	}

	uint32_t Parent = buffer[parent];

	if (!Parent || (Parent >> 17) == 0) {
		return false;
	}

	uint32_t FirstChild, Pointer;

	if (((Parent & 0x0000FF00) >> 8) == 0) { // Check if the parent still has no children
		if ((voxel >> 48) == 0) {
			return true;
		}

		Parent |= (0x01 << (8 + 7 - position)); // Update the parent's children mask

		FirstChild = allocBufferSpace(17);
		
		if (((FirstChild - parent) & 0xFFFFC000) && ((FirstChild - parent) & 0xFFFFC000) ^ 0xFFFFC000) { // Check if a pointer to the first child is necessary
			Pointer = allocBufferSpace(1);
			buffer[Pointer] = FirstChild;

			Parent = ((Parent & 0x0000FFFF) | 0x00010000 | ((Pointer - parent) & 0x00007FFF) << 17); // Update the parent's first child pointer
		}
		else {
			Parent = ((Parent & 0x0000FFFF) | ((FirstChild - parent) & 0x00007FFF) << 17); // Update the parent's first child pointer
		}

		buffer[parent] = Parent;
		buffer[FirstChild] = buffer[FirstChild + 1] = buffer[FirstChild + 2] = buffer[FirstChild + 3] = buffer[FirstChild + 4]
						   = buffer[FirstChild + 5] = buffer[FirstChild + 6] = buffer[FirstChild + 6] = buffer[FirstChild + 8]
						   = buffer[FirstChild + 9] = buffer[FirstChild + 10] = buffer[FirstChild + 11] = buffer[FirstChild + 12]
						   = buffer[FirstChild + 13] = buffer[FirstChild + 14] = buffer[FirstChild + 15] = 0x00000000;
		
		if (((parent - (FirstChild + 16)) & 0xFFFFC000) && ((parent - (FirstChild + 16)) & 0xFFFFC000) ^ 0xFFFFC000) { // Check if a pointer back to the parent is necessary
			Pointer = allocBufferSpace(1);
			buffer[Pointer] = parent;

			buffer[(FirstChild + 16)] = (0x00010001 | ((parent - Pointer) & 0x00007FFF) << 1); // Update the parent header's pointer
		}
		else {
			buffer[(FirstChild + 16)] = (0x00010000 | ((parent - (FirstChild + 16)) & 0x00007FFF) << 1); // Update the parent header's pointer
		}
	}
	else {
		FirstChild = parent + (int16_t)((Parent & 0x7FFE0000) >> 17 | (Parent & 0x80000000) >> 16 | (Parent & 0x80000000) >> 17);
		
		if (Parent & 0x00010000) {
			FirstChild = buffer[FirstChild];
		}

		if ((((Parent & 0x0000FF00) >> 8) & (0x01 << (7 - position))) > 0) { // Check if the parent already has a child at the position
			if (buffer[FirstChild + (position << 1)] == (voxel >> 32) && buffer[FirstChild + (position << 1) + 1] == (voxel & 0x00000000FFFFFFFF)) {
				return true; // Return true if the child hasn't changed
			}

			if ((voxel >> 17) > 0) {
				return false; // Return false if a child would have to be replaced
			}
			else {
				clearVoxel(FirstChild + position, true, true); // Clear the child

				return true;
			}
		}
		else if ((voxel >> 17) == 0) {
			return true;
		}

		Parent |= (0x01 << (8 + 7 - position)); // Update the parent's children mask

		buffer[parent] = Parent;
	}

	buffer[FirstChild + (position << 1)] = voxel >> 32; // Set the voxel in the buffer
	buffer[FirstChild + (position << 1) + 1] = voxel & 0x00000000FFFFFFFF; // Set the colour in the buffer
	
	return true;
}

bool VoxelBuffer::movVoxel(uint32_t parentF, uint8_t positionF, uint32_t parentT, uint8_t positionT) {
	if (positionF > 7 || positionT > 7) {
		return false;
	}

	uint32_t ParentF = buffer[parentF];
	uint32_t ParentT = buffer[parentT];

	if (!ParentF || !ParentT || (ParentF >> 17) == 0 || (ParentT >> 17) == 0) {
		return false;
	}

	if ((((ParentF & 0x0000FF00) >> 8) & (0x01 << (7 - positionF))) == 0) {
		return false; // Return false if parentF doesn't have a child at the position
	}
	
	uint8_t layerF = 1;
	uint8_t layerT = 1;

	uint32_t pointer = parentF;

	uint8_t i;

	while (pointer != 0) {
		i = 0;
		
		while ((buffer[pointer + i] >> 16) != 1) {
			i += 2;
		}

		pointer = pointer + i + (int16_t)((buffer[pointer + i] & 0x00007FFE) >> 1 | (buffer[pointer + i] & 0x00008000) | (buffer[pointer + i] & 0x00008000) >> 1); // Traverse the tree from parentF to the root

		if (buffer[pointer + i] & 0x00000001) {
			pointer = buffer[pointer];
		}

		layerF++;
	}

	pointer = parentT;

	while (pointer != 0) {
		i = 0;

		while ((buffer[pointer + i] >> 16) != 1) {
			i += 2;
		}

		pointer = pointer + i + (int16_t)((buffer[pointer + i] & 0x00007FFE) >> 1 | (buffer[pointer + i] & 0x00008000) | (buffer[pointer + i] & 0x00008000) >> 1); // Traverse the tree from parentT to the root

		layerT++;
	}

	if (layerT != layerF) {
		return false; // Return false if the voxel would have to change its layer
	}

	uint32_t voxel = parentF + (int16_t)((ParentF & 0x7FFE0000) >> 17 | (ParentF & 0x80000000) >> 16 | (ParentF & 0x80000000) >> 17);

	if (ParentF & 0x00010000) {
		voxel = buffer[voxel];
	}

	if (!setVoxel(parentT, positionT, ((uint64_t)(buffer[voxel] & 0xFFFF00FF) << 32) | buffer[voxel + 1])) {
		return false;
	}
	
	ParentT = buffer[parentT];

	if (((buffer[voxel] & 0x0000FF00) >> 8) > 0) {
		parentF = voxel + (int16_t)((buffer[voxel] & 0x7FFE0000) >> 17 | (buffer[voxel] & 0x80000000) >> 16 | (buffer[voxel] & 0x80000000) >> 17); // Pointer to the voxel's first child

		if (buffer[voxel] & 0x00010000) {
			parentF = buffer[parentF];
		}
		
		parentT = parentT + (int16_t)((ParentT & 0x7FFE0000) >> 17 | (ParentT & 0x80000000) >> 16 | (ParentT & 0x80000000) >> 17) + positionT; // Pointer to new position of the voxel
		
		if (ParentT & 0x00010000) {
			parentT = buffer[parentT];
		}
		
		if (buffer[parentT] & 0x00010000) {
			freeBufferSpace(parentT + (int16_t)((buffer[parentT] & 0x7FFE0000) >> 17 | (buffer[parentT] & 0x80000000) >> 16 | (buffer[parentT] & 0x80000000) >> 17), 1); // Free the previous pointer to the first child
		}
		
		if (((parentF - parentT) & 0xFFFFC000) && ((parentF - parentT) & 0xFFFFC000) ^ 0xFFFFC000) {
			pointer = allocBufferSpace(1);
			buffer[pointer] = parentF;

			buffer[parentT] = ((buffer[parentT] & 0x0000FFFF) | 0x00010000 | ((pointer - parentT) & 0x00007FFF) << 17); // Update the parent's first child pointer
		}
		else {
			buffer[parentT] = ((buffer[parentT] & 0x0000FFFF) | ((parentF - parentT) & 0x00007FFF) << 17); // Update the parent's first child pointer
		}

		if (buffer[parentF + 16] & 0x00000001) {
			freeBufferSpace(parentF + (int16_t)((buffer[parentF + 16] & 0x00007FFE) >> 1 | (buffer[parentF + 16] & 0x00008000) | (buffer[parentF + 16] & 0x00008000) >> 1), 1); // Free the previous pointer to the parent
		}
		
		if (((parentT - (parentF + 16)) & 0xFFFFC000) && ((parentT - (parentF + 16)) & 0xFFFFC000) ^ 0xFFFFC000) {
			pointer = allocBufferSpace(1);
			buffer[pointer] = parentT;

			buffer[(parentF + 16)] = (0x00010001 | ((parentT - pointer) & 0x00007FFF) << 1); // Update the parent header's pointer
		}
		else {
			buffer[(parentF + 16)] = (0x00010000 | ((parentT - (parentF + 16)) & 0x00007FFF) << 1); // Update the parent header's pointer
		}
	}

	clearVoxel(voxel, true, false); // Clear at the voxel's original position

	return true;
}

uint64_t VoxelBuffer::getVoxel(uint32_t index) {
	return (((index + 1) < size) ? (((uint64_t)buffer[index] << 32) | buffer[index + 1]) : 0x0000000000000000);
}

void VoxelBuffer::logVoxel(uint32_t index) {
	DEBUG_LOG_RAW("VoxelBuffer", "(%d) %s", index, std::bitset<64>(getVoxel(index)).to_string().c_str());
}

std::function<void(mat4)> VoxelBuffer::getRenderFunction(uint16_t width, uint16_t height, uint16_t fov, uint32_t* buffer, uint8_t* mask) {
	// TO DO:
	// - clipping algorithm
	// - regular frame drops
	// - mat4 optimisation
	
	if (fov < 1) fov = 1;
	if (fov > 360) fov = 360;
	
	double a = ((fov / 180.0) * M_PI) / 2.0;
	double ar = width / (double)height;

	mat4 perMat = mat4(1.0 / (ar * tan(a)), 0, 0, 0, 0, 1.0 / tan(a), 0, 0, 0, 0, -1, 0, 0, 0, -1, 0);
				//mat4(1.0 / (ar * tan(a)), 0, 0, 0, 0, 1.0 / tan(a), 0, 0, 0, 0, -1, -near, 0, 0, -1, 0);
	
	vec4* vs = (vec4*)malloc(sizeof(vec4) * 8);
	uint8_t* cc = (uint8_t*)malloc(sizeof(uint8_t) * 8);
	vec4* cv = (vec4*)malloc(sizeof(vec4) * 24);
	vec4* ch = (vec4*)malloc(sizeof(vec4) * 48);

	uint8_t allOutside;

	float frma, dsta, rela, resa;
	uint8_t msk, m;

	uint8_t* sm = (uint8_t*)malloc(sizeof(uint8_t) * height);

	mat4 perPro;
	uint32_t drawCounter, testCounter;

	vec2 min, max;

	int8_t i, j, k;

	vec4 tmp;

	bool c, draw;

	uint16_t sx, ex, x, y;

	vec2 di, dj, rx;

	uint32_t mcol;
	
	return [this, perMat, width, height, ar, vs, cc, cv, &allOutside, &frma, &dsta, &rela, &resa, &msk, &m, ch, buffer, mask, sm, &perPro, &drawCounter, &testCounter, &min, &max, &i, &j, &k, &tmp, &c, &draw, &sx, &ex, &x, &y, &di, &dj, &rx, &mcol](mat4 cm) {
		perPro = ((mat4)perMat) * cm.inverse();

		drawCounter = testCounter = 0;
		
		std::function<bool(int16_t, int16_t, int16_t, uint16_t, uint32_t, bool)> render = [this, perPro, width, height, ar, vs, cc, cv, &allOutside, &frma, &dsta, &rela, &resa, &msk, &m, ch, buffer, mask, sm, &drawCounter, &testCounter, &min, &max, &i, &j, &k, &tmp, &c, &draw, &sx, &ex, &x, &y, &di, &dj, &rx, &mcol](int16_t posX, int16_t posY, int16_t posZ, uint16_t size, uint32_t colour, bool children)->bool {
			// Project the voxel vertices into screen space -> if all of them are off return false
			// Check if the voxel is covered -> if so return false
			// Draw the pixels if the voxel is smaller than a pixel or hasn't got any children
			// Return whether the voxel hasn't been drawn

			// Bugs: - Disappearing cubes
			//		 - Aborting with depth
			//		 - Early clipping
			
			vs[0] = ((mat4)perPro) * vec4(posX - size, posY - size, posZ - size, 1);
			vs[1] = ((mat4)perPro) * vec4(posX + size, posY - size, posZ - size, 1);
			vs[2] = ((mat4)perPro) * vec4(posX - size, posY + size, posZ - size, 1);
			vs[3] = ((mat4)perPro) * vec4(posX + size, posY + size, posZ - size, 1);
			vs[4] = ((mat4)perPro) * vec4(posX - size, posY - size, posZ + size, 1);
			vs[5] = ((mat4)perPro) * vec4(posX + size, posY - size, posZ + size, 1);
			vs[6] = ((mat4)perPro) * vec4(posX - size, posY + size, posZ + size, 1);
			vs[7] = ((mat4)perPro) * vec4(posX + size, posY + size, posZ + size, 1);

			allOutside = 0xFF;

			for (i = 0; i < 8; ++i) {
				if (this->dot(4, vs[i]) < 0.0f) {
					cc[i] = (((vs[i].x < -1.0f) ? 0x01 : 0x00) |
							 ((vs[i].x > 1.0f) ? 0x02 : 0x00) |
							 ((vs[i].y < -1.0f) ? 0x04 : 0x00) |
							 ((vs[i].y > 1.0f) ? 0x08 : 0x00) |
							 (0x10));
				}
				else {
					cc[i] = (((vs[i].x < -vs[i].w) ? 0x01 : 0x00) |
							 ((vs[i].x > vs[i].w) ? 0x02 : 0x00) |
							 ((vs[i].y < -vs[i].w) ? 0x04 : 0x00) |
							 ((vs[i].y > vs[i].w) ? 0x08 : 0x00) |
							 (0x00));
				}

				allOutside &= cc[i];
			}

			if (allOutside != 0x00) {
				return false;
			}

			k = 0;

			msk = 0x00;

			for (i = 0; i < 8; ++i) {
				if (cc[i] == 0x00) {
					cv[k++] = vs[i];
					
					continue;
				}

				for (m = i ^ 0x01; (m ^ i) <= 0x04; m = i ^ ((m ^ i) << 1)) {
					if (cc[m] == 0x00) {
						tmp = vs[i];

						if (cc[i] & 0x10) {
							frma = this->dot(4, tmp);
							dsta = this->dot(4, vs[m]);
							rela = frma / (frma - dsta);

							tmp = this->lerp(tmp, vs[m], rela);
						}

						resa = 0.0f;

						y = 0x01;

						for (x = 0; x < 4; ++x) {
							if (cc[i] & y) {
								frma = this->dot(x, tmp);
								dsta = this->dot(x, vs[m]);
								rela = frma / (frma - dsta);

								if (resa < rela) resa = rela;
							}

							y <<= 1;
						}
						
						cv[k++] = this->lerp(tmp, vs[m], resa);

						if (cc[i] & 0xEF) {
							tmp = cv[k - 1];

							y = (((fabs(this->dot(0, tmp)) < M_EPSILON) ? 0x01 : 0x00) |
								((fabs(this->dot(1, tmp)) < M_EPSILON) ? 0x02 : 0x00) |
								((fabs(this->dot(2, tmp)) < M_EPSILON) ? 0x04 : 0x00) |
								((fabs(this->dot(3, tmp)) < M_EPSILON) ? 0x08 : 0x00));

							switch (cc[i] & 0xEF) {
								case 0x05: msk |= ((((y & 0x05) == 0x01) ? 0x80 : 0x00) | (((y & 0x05) == 0x04) ? 0x01 : 0x00)); break; // ld
								case 0x06: msk |= ((((y & 0x06) == 0x02) ? 0x40 : 0x00) | (((y & 0x06) == 0x04) ? 0x20 : 0x00)); break; // rd
								case 0x09: msk |= ((((y & 0x09) == 0x01) ? 0x04 : 0x00) | (((y & 0x09) == 0x08) ? 0x02 : 0x00)); break; // lu
								case 0x0A: msk |= ((((y & 0x0A) == 0x02) ? 0x08 : 0x00) | (((y & 0x0A) == 0x08) ? 0x10 : 0x00)); break; // ru
							}
						}
						
						continue;
					}

					if ((cc[i] & cc[m] & 0xEF) != 0x00) {
						switch (cc[i] & 0xEF) {
							case 0x05: msk |= ((cc[m] & 0x01) | (cc[m] & 0x04) << 5); break; // ld
							case 0x06: msk |= ((cc[m] & 0x02) << 4 | (cc[m] & 0x04) << 4); break; // rd
							case 0x09: msk |= ((cc[m] & 0x01) << 1 | (cc[m] & 0x08) >> 1); break; // lu
							case 0x0A: msk |= ((cc[m] & 0x02) << 3 | (cc[m] & 0x08)); break; // ru
						}

						continue;
					}

					tmp = vs[i];

					if (cc[i] & 0x10) {
						frma = this->dot(4, tmp);
						dsta = this->dot(4, vs[m]);
						rela = frma / (frma - dsta);

						tmp = this->lerp(tmp, vs[m], rela);
					}

					if (!(cc[i] & cc[m] & 0x10)) {
						resa = 0.0f;

						y = 0x01;

						for (x = 0; x < 4; ++x) {
							if (cc[i] & y) {
								frma = this->dot(x, tmp);
								dsta = this->dot(x, vs[m]);
								rela = frma / (frma - dsta);

								if (frma > dsta) break;

								if (resa < rela) resa = rela;
							}

							y <<= 1;
						}
					}

					if ((cc[i] & cc[m] & 0x10) || x < 4) {
						if (cc[i] & 0x01) {
							if (cc[m] & 0x04) {
								msk |= 0x81;
							}
							else if (cc[m] & 0x08) {
								msk |= 0x06;
							}
						}
						else if (cc[i] & 0x02) {
							if (cc[m] & 0x04) {
								msk |= 0x60;
							}
							else if (cc[m] & 0x08) {
								msk |= 0x18;
							}
						}
						else if (cc[i] & 0x04) {
							if (cc[m] & 0x01) {
								msk |= 0x81;
							}
							else if (cc[m] & 0x02) {
								msk |= 0x60;
							}
						}
						else if (cc[i] & 0x08) {
							if (cc[m] & 0x01) {
								msk |= 0x06;
							}
							else if (cc[m] & 0x02) {
								msk |= 0x18;
							}
						}
					}
					else {
						cv[k++] = this->lerp(vs[i], vs[m], resa);
					}
				}
			}

			//DEBUG_LOG_RAW("Mask", "%s", std::bitset<8>(msk).to_string().c_str());

			if ((msk & 0x81) == 0x81) {
				cv[k++] = vec4(-1, -1, 1, 1);
			}
			if ((msk & 0x60) == 0x60) {
				cv[k++] = vec4(1, -1, 1, 1);
			}
			if ((msk & 0x06) == 0x06) {
				cv[k++] = vec4(-1, 1, 1, 1);
			}
			if ((msk & 0x18) == 0x18) {
				cv[k++] = vec4(1, 1, 1, 1);
			}

			min = vec2(width - 1, height - 1);
			max = vec2(0, 0);

			for (i = 0; i < k; ++i) {
				cv[i].x = ROUND_2_INT((1.0 + cv[i].x / cv[i].w) * 0.5 * (width - 1.0));
				cv[i].y = ROUND_2_INT((1.0 + cv[i].y / cv[i].w) * 0.5 * (height - 1.0));

				//DEBUG_LOG_RAW("Render", "(%d|%d)", (int)cv[i].x, (int)cv[i].y);

				min.x = MIN(min.x, cv[i].x);
				max.x = MAX(max.x, cv[i].x);
				min.y = MIN(min.y, cv[i].y);
				max.y = MAX(max.y, cv[i].y);
			}

			bool draw = !children || ((max.x - min.x) < 2 && (max.y - min.y) < 2);

			if (!draw && ((max.x - min.x) * (max.y - min.y)) > drawCounter) {
				return true;
			}

			if (drawCounter >= (width * height)) {
				return false;
			}

			for (i = 0; i < (k - 1); ++i) {
				for (j = 0; j < (k - 1 - i); ++j) {
					if (!(cv[j].x < cv[j+1].x || (cv[j].x == cv[j+1].x && cv[j].y < cv[j+1].y))) {
						tmp = cv[j + 1];
						cv[j + 1] = cv[j];
						cv[j] = tmp;
					}
				}
			}

			x = k;
			k = 0;
			
			for (i = 0; i < x; i++) {
				while (k >= 2 && ((ch[k - 1].x - ch[k - 2].x) * (cv[i].y - ch[k - 2].y) - (ch[k - 1].y - ch[k - 2].y) * (cv[i].x - ch[k - 2].x)) <= 0) k--;

				ch[k++] = cv[i];
			}
			
			for (i = x - 2, j = k + 1; i >= 0; i--) {
				while (k >= j && ((ch[k - 1].x - ch[k - 2].x) * (cv[i].y - ch[k - 2].y) - (ch[k - 1].y - ch[k - 2].y) * (cv[i].x - ch[k - 2].x)) <= 0) k--;
				
				ch[k++] = cv[i];
			}
			
			k -= 1;

			for (j = 0; j < k; ++j) {
				if (ch[j].y == min.y) break;
			}

			i = (j + k - 1) % k;
			j = (j + 1) % k;

			di = vec2(ch[i].x - ch[(i + 1) % k].x, ch[i].y - ch[(i + 1) % k].y);
			dj = vec2(ch[j].x - ch[(j + k - 1) % k].x, ch[j].y - ch[(j + k - 1) % k].y);

			testCounter++;
			
			for (y = min.y; y <= max.y; ++y) {
				if (sm[y] < width && ((ch[i].y >= y) != (ch[(i + 1) % k].y >= y)) && ((ch[j].y >= y) != (ch[(j + k - 1) % k].y >= y))) {
					rx.x = ((di.y == 0.0) ? ch[i].x : (ch[(i + 1) % k].x + (y - ch[(i + 1) % k].y) * di.x / di.y));
					rx.y = ((dj.y == 0.0) ? ch[j].x : (ch[(j + k - 1) % k].x + (y - ch[(j + k - 1) % k].y) * dj.x / dj.y));

					testCounter += 2;

					if (rx.y < rx.x) {
						sx = (uint16_t)MAX(CEILN(rx.y), 0.0);
						ex = (uint16_t)MIN(floor(rx.x), width - 1.0);
					}
					else {
						sx = (uint16_t)MAX(CEILN(rx.x), 0.0);
						ex = (uint16_t)MIN(floor(rx.y), width - 1.0);
					}
					
					for (x = sx; x <= ex; ++x) {
						if (mask[y * width + x] != 0xFF) {
							if (!draw) {
								return true;
							}

							mcol = ((buffer[y * width + x] == 0x00000000) ? colour : colourMix(buffer[y * width + x], colour));

							mask[y * width + x] = mcol & 0x000000FF;
							buffer[y * width + x] = mcol;

							sm[y] += ((mcol & 0x000000FF) == 255);

							drawCounter++;
						}
					}
				}

				if (ch[i].y == y) {
					i = (i + k - 1) % k;

					di = vec2(ch[i].x - ch[(i + 1) % k].x, ch[i].y - ch[(i + 1) % k].y);
				}

				if (ch[j].y == y) {
					j = (j + 1) % k;

					dj = vec2(ch[j].x - ch[(j + k - 1) % k].x, ch[j].y - ch[(j + k - 1) % k].y);
				}
			}
			
			return false;
		};

		memset(buffer, 0x0, width * height * 4);
		memset(mask, 0x0, width * height);
		memset(sm, 0x0, height);

		if (this->buffer[1] & 0x80000000) {
			this->updateColours(0);
		}
		
		this->frontToBack(0x00000000, 0, 0, 0, 0x8000, (float)cm[3], (float)cm[7], (float)cm[11], render);
	};
}

void VoxelBuffer::frontToBack(uint32_t index, int16_t posX, int16_t posY, int16_t posZ, uint16_t size, const float eyeX, const float eyeY, const float eyeZ, std::function<bool(int16_t, int16_t, int16_t, uint16_t, uint32_t, bool)>& render) {
	uint8_t children = (buffer[index] & 0x0000FF00) >> 8;
	uint8_t first = ((eyeX > posX) ? 1 : 0) | ((eyeY > posY) ? 2 : 0) | ((eyeZ > posZ) ? 4 : 0);

	if (render(posX, posY, posZ, size, buffer[index + 1], children)) {
		uint32_t firstChild = index + (int16_t)((buffer[index] & 0x7FFE0000) >> 17 | (buffer[index] & 0x80000000) >> 16 | (buffer[index] & 0x80000000) >> 17);
		
		size >>= 1;

		if (buffer[index] & 0x00010000) {
			firstChild = buffer[firstChild];
		}

		if (children & (0x01 << (7 - first))) {
			frontToBack(firstChild + (first << 1), posX + ((first & 0x01) ? size : -size), posY + ((first & 0x02) ? size : -size), posZ + ((first & 0x04) ? size : -size), size, eyeX, eyeY, eyeZ, render);
		}
		if (children & (0x01 << (7 - (first ^ 1)))) {
			frontToBack(firstChild + ((first ^ 1) << 1), posX + (((first ^ 1) & 0x01) ? size : -size), posY + (((first ^ 1) & 0x02) ? size : -size), posZ + (((first ^ 1) & 0x04) ? size : -size), size, eyeX, eyeY, eyeZ, render);
		}
		if (children & (0x01 << (7 - (first ^ 2)))) {
			frontToBack(firstChild + ((first ^ 2) << 1), posX + (((first ^ 2) & 0x01) ? size : -size), posY + (((first ^ 2) & 0x02) ? size : -size), posZ + (((first ^ 2) & 0x04) ? size : -size), size, eyeX, eyeY, eyeZ, render);
		}
		if (children & (0x01 << (7 - (first ^ 4)))) {
			frontToBack(firstChild + ((first ^ 4) << 1), posX + (((first ^ 4) & 0x01) ? size : -size), posY + (((first ^ 4) & 0x02) ? size : -size), posZ + (((first ^ 4) & 0x04) ? size : -size), size, eyeX, eyeY, eyeZ, render);
		}
		if (children & (0x01 << (7 - (first ^ 3)))) {
			frontToBack(firstChild + ((first ^ 3) << 1), posX + (((first ^ 3) & 0x01) ? size : -size), posY + (((first ^ 3) & 0x02) ? size : -size), posZ + (((first ^ 3) & 0x04) ? size : -size), size, eyeX, eyeY, eyeZ, render);
		}
		if (children & (0x01 << (7 - (first ^ 5)))) {
			frontToBack(firstChild + ((first ^ 5) << 1), posX + (((first ^ 5) & 0x01) ? size : -size), posY + (((first ^ 5) & 0x02) ? size : -size), posZ + (((first ^ 5) & 0x04) ? size : -size), size, eyeX, eyeY, eyeZ, render);
		}
		if (children & (0x01 << (7 - (first ^ 6)))) {
			frontToBack(firstChild + ((first ^ 6) << 1), posX + (((first ^ 6) & 0x01) ? size : -size), posY + (((first ^ 6) & 0x02) ? size : -size), posZ + (((first ^ 6) & 0x04) ? size : -size), size, eyeX, eyeY, eyeZ, render);
		}
		if (children & (0x01 << (7 - (first ^ 7)))) {
			frontToBack(firstChild + ((first ^ 7) << 1), posX + (((first ^ 7) & 0x01) ? size : -size), posY + (((first ^ 7) & 0x02) ? size : -size), posZ + (((first ^ 7) & 0x04) ? size : -size), size, eyeX, eyeY, eyeZ, render);
		}
	}
}

bool VoxelBuffer::addVoxel(int16_t posX, int16_t posY, int16_t posZ, uint16_t size, uint64_t voxel) {
	uint16_t csize = 0x8000;
	
	if (posX < (-csize + 1) || posX >(csize - 1) || posY < (-csize + 1) || posY >(csize - 1) || posZ < (-csize + 1) || posZ >(csize - 1)) {
		return false;
	}

	if (!(size == 0x0001 || size == 0x0002 || size == 0x0004 || size == 0x0008 || size == 0x0010 || size == 0x0020 || size == 0x0040 || size == 0x0080 || size == 0x0100 || size == 0x0200 || size == 0x0400 || size == 0x0800 || size == 0x1000 || size == 0x2000 || size == 0x4000)) {
		return false;
	}

	csize >>= 1;

	int16_t cposX = 0, cposY = 0, cposZ = 0;

	uint32_t index = 0;
	uint8_t child = 0;

	uint64_t defVox = VoxelBuffer::constructVoxel(0x00000000);

	while (csize > size) {
		child = ((posX < cposX) ? 0 : 1) + ((posY < cposY) ? 0 : 2) + ((posZ < cposZ) ? 0 : 4);

		setVoxel(index, child, defVox);

		buffer[index + 1] |= 0x80000000;

		index = index + (int16_t)((buffer[index] & 0x7FFE0000) >> 17 | (buffer[index] & 0x80000000) >> 16 | (buffer[index] & 0x80000000) >> 17) + (child << 1);
		
		cposX += (posX < cposX) ? -csize : csize;
		cposY += (posY < cposY) ? -csize : csize;
		cposZ += (posZ < cposZ) ? -csize : csize;
		
		csize >>= 1;
	}

	buffer[index + 1] |= 0x80000000;

	child = ((posX < cposX) ? 0 : 1) + ((posY < cposY) ? 0 : 2) + ((posZ < cposZ) ? 0 : 4);

	return setVoxel(index, child, voxel);
}

void VoxelBuffer::updateColours(uint32_t index) {
	buffer[index + 1] &= 0x7FFFFFFF;
	
	uint8_t children = (buffer[index] & 0x0000FF00) >> 8;

	if (!children) {
		return;
	}

	uint32_t firstChild = index + (int16_t)((buffer[index] & 0x7FFE0000) >> 17 | (buffer[index] & 0x80000000) >> 16 | (buffer[index] & 0x80000000) >> 17);

	if (buffer[index] & 0x00010000) {
		firstChild = buffer[firstChild];
	}

	uint32_t colours[8];
	uint8_t k = 0;

	for (uint8_t i = 0; i < 8; ++i) {
		if (children & (0x01 << (7 - i))) {
			if (buffer[firstChild + i * 2 + 1] & 0x80000000) updateColours(firstChild + i * 2);

			colours[k++] = buffer[firstChild + i * 2 + 1];
		}
	}

	for (uint8_t i = 1; i < k; ++i) {
		colours[0] = colourMix(colours[0], colours[i]);
	}

	buffer[index + 1] = colours[0];
}

float VoxelBuffer::dot(uint8_t plane, vec4& v) {
	// v.w has to be positive
	
	switch (plane) {
		case 0: return v.x + v.w;
		case 1: return -v.x + v.w;
		case 2: return v.y + v.w;
		case 3: return -v.y + v.w;
		case 4: return v.w - 1.0f;
		default: return 0.0f;
	}
}

vec4 VoxelBuffer::lerp(vec4& a, vec4& b, float alpha) {
	float aAlpha = 1.0f - alpha;

	return vec4(aAlpha * a.x + alpha * b.x, aAlpha * a.y + alpha * b.y, aAlpha * a.z + alpha * b.z, aAlpha * a.w + alpha * b.w);
}