#pragma once

#include <cstdint>

template<uint8_t opcode, size_t size, uint8_t amount> struct Operation {
	static const uint8_t opc = opcode;
	static const size_t siz = size;
	static const uint8_t amt = amount;
};

struct BUF_END : Operation<0b0000, 0, std::numeric_limits<uint8_t>::max()>{

};

struct VOX_LOC : Operation<0b0001, 3 + 8 * ((3 + 1) + (8 + 4 + 4)), 8> {

};

struct VOX_COL : Operation<0b0010, 3 + (3 + 8), 1> {

};

struct VOX_CPY : Operation<0b0011, 3 + 3, 1> {

};

struct STR_LOC : Operation<0b0100, 3 + 8 * (3 + 1), 8> {

};

struct STR_LOS : Operation<0b0101, 3 + (3 + 1), 1> {

};

struct STR_CLD : Operation<0b0110, 3 + 1, 1> {

};

struct STR_MOV : Operation<0b0111, 3 + 3, 1> {

};

struct MAT_LOC : Operation<0b1000, 3 + 8 * (8 + 4 + 4), 8> {

};

struct MAT_xxL : Operation<0b1001, 3 + 4, 1> {

};

struct MAT_xSx : Operation<0b1010, 3 + 4, 1> {

};

struct MAT_xSL : Operation<0b1011, 3 + (4 + 4), 1> {

};

struct MAT_Cxx : Operation<0b1100, 3 + 8, 1> {

};

struct MAT_CxL : Operation<0b1101, 3 + (8 + 4), 1> {

};

struct MAT_CSx : Operation<0b1110, 3 + (8 + 4), 1> {

};

struct MAT_CSL : Operation<0b1111, 3 + (8 + 4 + 4), 1> {

};