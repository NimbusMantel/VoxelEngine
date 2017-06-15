#include "instruct.hpp"

/*KERNEL_INCLUDE_BEG*/
enum INS_CTG_C {
	INS_CTG_RLD_C,
	INS_CTG_ULD_C,
	INS_CTG_ADD_C,
	INS_CTG_REM_C,
	INS_CTG_MOV_C,
	INS_CTG_EXP_C,
	INS_CTG_EMI_C,
	INS_CTG_COL_C,
	INS_CTG_LIT_C
};

enum INS_CTG_S {
	INS_CTG_RLD_S = 136,
	INS_CTG_ULD_S = 4,
	INS_CTG_ADD_S = 133,
	INS_CTG_REM_S = 5,
	INS_CTG_MOV_S = 9,
	INS_CTG_EXP_S = 8,
	INS_CTG_EMI_S = 5,
	INS_CTG_COL_S = 6,
	INS_CTG_LIT_S = 12
};

enum INS_GTC_C {
	INS_GTC_REQ_C,
	INS_GTC_SUG_C
};

enum INS_GTC_S {
	INS_GTC_REQ_S = 4,
	INS_GTC_SUG_S = 4
};
/*KERNEL_INCLUDE_END*/

enum INS_CTG_P {
	INS_CTG_RLD_P = 1,
	INS_CTG_ULD_P = 2,
	INS_CTG_ADD_P = 3,
	INS_CTG_REM_P = 5,
	INS_CTG_MOV_P = 6,
	INS_CTG_EXP_P = 4,
	INS_CTG_EMI_P = 0,
	INS_CTG_COL_P = 0,
	INS_CTG_LIT_P = 0
};

enum INS_GTC_P {
	INS_GTC_REQ_P = 0,
	INS_GTC_SUG_P = 1
};

static const uint8_t ins_ctg_all[INS_CTG::NUM] = { INS_CTG_MOV_C, INS_CTG_REM_C, INS_CTG_EXP_C, INS_CTG_ADD_C, INS_CTG_ULD_C, INS_CTG_RLD_C, INS_CTG_EMI_C, INS_CTG_COL_C, INS_CTG_LIT_C };
const uint8_t* INS_CTG::ALL = ins_ctg_all;

const uint8_t INS_CTG_RLD::opc = INS_CTG_RLD_C; const uint32_t INS_CTG_RLD::siz = INS_CTG_RLD_S; const uint8_t INS_CTG_RLD::pri = INS_CTG_RLD_P;
const uint8_t INS_CTG_ULD::opc = INS_CTG_ULD_C; const uint32_t INS_CTG_ULD::siz = INS_CTG_ULD_S; const uint8_t INS_CTG_ULD::pri = INS_CTG_ULD_P;
const uint8_t INS_CTG_ADD::opc = INS_CTG_ADD_C; const uint32_t INS_CTG_ADD::siz = INS_CTG_ADD_S; const uint8_t INS_CTG_ADD::pri = INS_CTG_ADD_P;
const uint8_t INS_CTG_REM::opc = INS_CTG_REM_C; const uint32_t INS_CTG_REM::siz = INS_CTG_REM_S; const uint8_t INS_CTG_REM::pri = INS_CTG_REM_P;
const uint8_t INS_CTG_MOV::opc = INS_CTG_MOV_C; const uint32_t INS_CTG_MOV::siz = INS_CTG_MOV_S; const uint8_t INS_CTG_MOV::pri = INS_CTG_MOV_P;
const uint8_t INS_CTG_EXP::opc = INS_CTG_EXP_C; const uint32_t INS_CTG_EXP::siz = INS_CTG_EXP_S; const uint8_t INS_CTG_EXP::pri = INS_CTG_EXP_P;
const uint8_t INS_CTG_EMI::opc = INS_CTG_EMI_C; const uint32_t INS_CTG_EMI::siz = INS_CTG_EMI_S; const uint8_t INS_CTG_EMI::pri = INS_CTG_EMI_P;
const uint8_t INS_CTG_COL::opc = INS_CTG_COL_C; const uint32_t INS_CTG_COL::siz = INS_CTG_COL_S; const uint8_t INS_CTG_COL::pri = INS_CTG_COL_P;
const uint8_t INS_CTG_LIT::opc = INS_CTG_LIT_C; const uint32_t INS_CTG_LIT::siz = INS_CTG_LIT_S; const uint8_t INS_CTG_LIT::pri = INS_CTG_LIT_P;

// TO DO: Implement the different writing operations

void INS_CTG_RLD::WRI(uint8_t* buf) {
	return;
}

void INS_CTG_ULD::WRI(uint8_t* buf) {
	return;
}

void INS_CTG_ADD::WRI(uint8_t* buf) {
	return;
}

void INS_CTG_REM::WRI(uint8_t* buf) {
	return;
}

void INS_CTG_MOV::WRI(uint8_t* buf) {
	return;
}

void INS_CTG_EXP::WRI(uint8_t* buf) {
	return;
}

void INS_CTG_EMI::WRI(uint8_t* buf) {
	return;
}

void INS_CTG_COL::WRI(uint8_t* buf) {
	return;
}

void INS_CTG_LIT::WRI(uint8_t* buf) {
	return;
}