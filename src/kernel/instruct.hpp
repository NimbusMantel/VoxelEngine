#pragma once

#include <cstdint>

typedef bool uint1_t;

struct INS_CTG { static const uint8_t num; static const uint8_t* all; bool syn; INS_CTG(bool s) : syn(s) {} };

struct INS_CTG_RLD : INS_CTG { static const uint8_t opc; static const uint32_t siz; static const uint8_t pri; uint32_t parent; uint32_t index; uint32_t(&children)[32]; INS_CTG_RLD(uint32_t p, uint32_t i, uint32_t(&c)[32], uint8_t s = false) : INS_CTG(s), parent(p), index(i), children(c) {} };
struct INS_CTG_ULD : INS_CTG { static const uint8_t opc; static const uint32_t siz; static const uint8_t pri; uint32_t parent; INS_CTG_ULD(uint32_t p, uint8_t s = false) : INS_CTG(s), parent(p) {} };
struct INS_CTG_ADD : INS_CTG { static const uint8_t opc; static const uint32_t siz; static const uint8_t pri; uint32_t parent; uint8_t mask; uint32_t (&children)[32]; INS_CTG_ADD(uint32_t p, uint8_t m, uint32_t(&c)[32], uint8_t s = false) : INS_CTG(s), parent(p), mask(m), children(c) {} };
struct INS_CTG_REM : INS_CTG { static const uint8_t opc; static const uint32_t siz; static const uint8_t pri; uint32_t parent; uint8_t mask; INS_CTG_REM(uint32_t p, uint8_t m, uint8_t s = false) : INS_CTG(s), parent(p), mask(m) {} };
struct INS_CTG_MOV : INS_CTG { static const uint8_t opc; static const uint32_t siz; static const uint8_t pri; uint32_t fparent; uint8_t fidx; uint32_t tparent; uint8_t tidx; INS_CTG_MOV(uint32_t fp, uint8_t fi, uint32_t tp, uint8_t ti, uint8_t s = false) : INS_CTG(s), fparent(fp), fidx(fi), tparent(tp), tidx(ti) {} };
struct INS_CTG_EXP : INS_CTG { static const uint8_t opc; static const uint32_t siz; static const uint8_t pri; uint32_t parent; uint32_t index; INS_CTG_EXP(uint32_t p, uint32_t i, uint8_t s = false) : INS_CTG(s), parent(p), index(i) {} };
struct INS_CTG_EMI : INS_CTG { static const uint8_t opc; static const uint32_t siz; static const uint8_t pri; uint32_t index; uint1_t emitter; INS_CTG_EMI(uint32_t i, uint1_t e, uint8_t s = false) : INS_CTG(s), index(i), emitter(e) {} };
struct INS_CTG_COL : INS_CTG { static const uint8_t opc; static const uint32_t siz; static const uint8_t pri; uint32_t index; uint16_t colour; INS_CTG_COL(uint32_t i, uint16_t c, uint8_t s = false) : INS_CTG(s), index(i), colour(c) {} };
struct INS_CTG_LIT : INS_CTG { static const uint8_t opc; static const uint32_t siz; static const uint8_t pri; uint32_t index; uint16_t (&lights)[3]; INS_CTG_LIT(uint32_t i, uint16_t (&l)[3], uint8_t s = false) : INS_CTG(s), index(i), lights(l) {} };