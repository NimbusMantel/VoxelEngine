#include "instruct.hpp"

#define INT32_BUF(i, b, p) b[p] = ((i) & 0xFF000000) >> 24; b[p + 1] = ((i) & 0x00FF0000) >> 16; b[p + 2] = ((i) & 0x0000FF00) >> 8; b[p + 3] = ((i) & 0x000000FF);

/*KERNEL_INSTRUCT_BEG*/
enum INS_CTG_C {
	INS_CTG_RLD_C,
	INS_CTG_ULD_C,
	INS_CTG_ADD_C,
	INS_CTG_REM_C,
	INS_CTG_MOV_C,
	INS_CTG_EXP_C,
	INS_CTG_MAT_C,
	INS_CTG_LIT_C
};

enum INS_CTG_S {
	INS_CTG_RLD_S = 136,
	INS_CTG_ULD_S = 4,
	INS_CTG_ADD_S = 36,
	INS_CTG_REM_S = 5,
	INS_CTG_MOV_S = 9,
	INS_CTG_EXP_S = 8,
	INS_CTG_MAT_S = 8,
	INS_CTG_LIT_S = 8
};

enum INS_GTC_C {
	INS_GTC_REQ_C,
	INS_GTC_SUG_C
};

enum INS_GTC_S {
	INS_GTC_REQ_S = 4,
	INS_GTC_SUG_S = 4
};
/*KERNEL_INSTRUCT_END*/

enum INS_CTG_P {
	INS_CTG_RLD_P = 1,
	INS_CTG_ULD_P = 2,
	INS_CTG_ADD_P = 3,
	INS_CTG_REM_P = 5,
	INS_CTG_MOV_P = 6,
	INS_CTG_EXP_P = 4,
	INS_CTG_MAT_P = 0,
	INS_CTG_LIT_P = 0
};

enum INS_GTC_P {
	INS_GTC_REQ_P = 0,
	INS_GTC_SUG_P = 1
};

static const uint8_t ins_ctg_all[INS_CTG::NUM] = { INS_CTG_MOV_C, INS_CTG_REM_C, INS_CTG_EXP_C, INS_CTG_ADD_C, INS_CTG_ULD_C, INS_CTG_RLD_C, INS_CTG_MAT_C, INS_CTG_LIT_C };
const uint8_t* INS_CTG::ALL = ins_ctg_all;

const uint8_t INS_CTG_RLD::opc = INS_CTG_RLD_C; const uint32_t INS_CTG_RLD::siz = INS_CTG_RLD_S; const uint8_t INS_CTG_RLD::pri = INS_CTG_RLD_P;
const uint8_t INS_CTG_ULD::opc = INS_CTG_ULD_C; const uint32_t INS_CTG_ULD::siz = INS_CTG_ULD_S; const uint8_t INS_CTG_ULD::pri = INS_CTG_ULD_P;
const uint8_t INS_CTG_ADD::opc = INS_CTG_ADD_C; const uint32_t INS_CTG_ADD::siz = INS_CTG_ADD_S; const uint8_t INS_CTG_ADD::pri = INS_CTG_ADD_P;
const uint8_t INS_CTG_REM::opc = INS_CTG_REM_C; const uint32_t INS_CTG_REM::siz = INS_CTG_REM_S; const uint8_t INS_CTG_REM::pri = INS_CTG_REM_P;
const uint8_t INS_CTG_MOV::opc = INS_CTG_MOV_C; const uint32_t INS_CTG_MOV::siz = INS_CTG_MOV_S; const uint8_t INS_CTG_MOV::pri = INS_CTG_MOV_P;
const uint8_t INS_CTG_EXP::opc = INS_CTG_EXP_C; const uint32_t INS_CTG_EXP::siz = INS_CTG_EXP_S; const uint8_t INS_CTG_EXP::pri = INS_CTG_EXP_P;
const uint8_t INS_CTG_MAT::opc = INS_CTG_MAT_C; const uint32_t INS_CTG_MAT::siz = INS_CTG_MAT_S; const uint8_t INS_CTG_MAT::pri = INS_CTG_MAT_P;
const uint8_t INS_CTG_LIT::opc = INS_CTG_LIT_C; const uint32_t INS_CTG_LIT::siz = INS_CTG_LIT_S; const uint8_t INS_CTG_LIT::pri = INS_CTG_LIT_P;

void INS_CTG_RLD::WRI(uint8_t* buf) {
	INT32_BUF(parent, buf, 0);

	INT32_BUF(index, buf, 4);

	for (uint8_t i = 0; i < 32; ++i) {
		INT32_BUF(children[i], buf, 8 + i * 4);
	}
}

void INS_CTG_ULD::WRI(uint8_t* buf) {
	INT32_BUF(parent, buf, 0);
}

void INS_CTG_ADD::WRI(uint8_t* buf) {
	INT32_BUF(parent, buf, 0);
	
	for (uint8_t i = 0; i < 8; ++i) {
		INT32_BUF(children[i], buf, 4 + i * 4);
	}
}

void INS_CTG_REM::WRI(uint8_t* buf) {
	INT32_BUF(parent, buf, 0);

	buf[4] = mask;
}

void INS_CTG_MOV::WRI(uint8_t* buf) {
	INT32_BUF(fparent, buf, 0);

	INT32_BUF(tparent, buf, 4);

	buf[8] = (((fidx & 0x07) << 5) | ((tidx & 0x07) << 2));
}

void INS_CTG_EXP::WRI(uint8_t* buf) {
	INT32_BUF(parent, buf, 0);

	INT32_BUF(index, buf, 4);
}

void INS_CTG_MAT::WRI(uint8_t* buf) {
	INT32_BUF(index, buf, 0);

	INT32_BUF(material, buf, 4);
}

void INS_CTG_LIT::WRI(uint8_t* buf) {
	INT32_BUF(index, buf, 0);

	INT32_BUF(light, buf, 4);
}