#pragma once

#include <cstdint>

/*KERNEL_INCLUDE_BEG*/
enum INS_CTG_C {
	INS_CTG_RLD_C = 0x01,
	INS_CTG_ULD_C = 0x02,
	INS_CTG_ADD_C = 0x03,
	INS_CTG_REM_C = 0x04,
	INS_CTG_MOV_C = 0x05,
	INS_CTG_EMI_C = 0x06,
	INS_CTG_COL_C = 0x07,
	INS_CTG_LIT_C = 0x08
};

enum INS_CTG_S {
	INS_CTG_RLD_S = 36,
	INS_CTG_ULD_S = 4,
	INS_CTG_ADD_S = 37,
	INS_CTG_REM_S = 5,
	INS_CTG_MOV_S = 9,
	INS_CTG_EMI_S = 5,
	INS_CTG_COL_S = 6,
	INS_CTG_LIT_S = 12
};

enum INS_GTC_C {
	INS_GTC_REQ_C = 0x01,
	INS_GTC_SUG_C = 0x02
};

enum INS_GTC_S {
	INS_GTC_REQ_S = 4,
	INS_GTC_SUG_S = 4
};
/*KERNEL_INCLUDE_END*/