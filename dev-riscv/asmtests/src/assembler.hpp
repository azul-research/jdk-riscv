#ifdef _ASSEMBLER_H_INCLUDED_
#error "you're doing something wrong, just use `make check`"
#endif
#define _ASSEMBLER_H_INCLUDED_

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>

#define __
#define TEST(name) AvailableTests.push_back(name); if (CurrentTestName == name)

typedef int Register;

extern std::string CurrentTestName;
extern std::vector<std::string> AvailableTests;

const char *regnames[] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",
    "s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",
    "t3", "t4", "t5", "t6",

    "%0", "%1", "%2", "%3", "%4"
};

enum {
    R0 = 0,
    R0_ZERO = 0,
    R1 = 1,
    R2 = 2,
    R3 = 3,
    R4 = 4,
    R5 = 5,
    R6 = 6,
    R7 = 7,
    R8 = 8,
    R9 = 9,
    R10 = 10,
    R11 = 11,
    R12 = 12,
    R13 = 13,
    R14 = 14,
    R15 = 15,
    R16 = 16,
    R17 = 17,
    R18 = 18,
    R19 = 19,
    R20 = 20,
    R21 = 21,
    R22 = 22,
    R23 = 23,
    R24 = 24,
    R25 = 25,
    R26 = 26,
    R27 = 27,
    R28 = 28,
    R29 = 29,
    R30 = 30,
    R31 = 31,

    Rtemplate0 = 32,
    Rtemplate1 = 33,
    Rtemplate2 = 34,
    Rtemplate3 = 35,
    Rtemplate4 = 36,
};

struct Assembler {
    std::FILE *output;
    uint64_t _pc = 0x1000000000000000ull; // 2**60 TODO: try to implement this better

    inline uint64_t pc() {
        return _pc;
    }

    inline void u(const char *instr, Register d, int imm) {
        std::fprintf(output, "\"%s %s, 0x%x;\\n\"\n", instr, regnames[d], imm);
        _pc += 4;
    }

    inline void i(const char *instr, Register d, Register s, int imm) {
        unsigned t = (unsigned)imm & 0xfff;
        if (t >= 0x800) {
            imm = (int)t - 0x1000;
        } else {
            imm = t;
        }
        std::fprintf(output, "\"%s %s, %s, %d;\\n\"\n", instr, regnames[d], regnames[s], imm);
        _pc += 4;
    }

    inline void r(const char *instr, Register d, Register s1, Register s2) {
        std::fprintf(output, "\"%s %s, %s, %s;\\n\"\n", instr, regnames[d], regnames[s1], regnames[s2]);
        _pc += 4;
    }

    inline void lui(      Register d,  int imm) { u("lui", d, imm); }
    inline void auipc(    Register d,  int imm) { u("auipc", d, imm); }

    inline void addi(Register d, Register s, int imm) { i("addi", d, s, imm); }
    inline void addiw(Register d, Register s, int imm) { i("addiw", d, s, imm); }
    inline void slti(Register d, Register s, int imm) { i("slti", d, s, imm); }
    inline void sltiu(Register d, Register s, int imm) { i("sltiu", d, s, imm); }
    inline void xori(Register d, Register s, int imm) { i("xori", d, s, imm); }
    inline void ori(Register d, Register s, int imm) { i("ori", d, s, imm); }
    inline void andi(Register d, Register s, int imm) { i("andi", d, s, imm); }
    inline void slli(Register d, Register s, int imm) { i("slli", d, s, imm); }
    inline void srli(Register d, Register s, int imm) { i("srli", d, s, imm); }
    inline void srai(Register d, Register s, int imm) { i("srai", d, s, imm); }

    inline void add(Register d, Register s1, int s2) { r("add", d, s1, s2); }
    inline void slt(Register d, Register s1, int s2) { r("slt", d, s1, s2); }
    inline void sltu(Register d, Register s1, int s2) { r("sltu", d, s1, s2); }
    inline void andr(Register d, Register s1, int s2) { r("andr", d, s1, s2); }
    inline void orr(Register d, Register s1, int s2) { r("orr", d, s1, s2); }
    inline void xorr(Register d, Register s1, int s2) { r("xorr", d, s1, s2); }
    inline void sll(Register d, Register s1, int s2) { r("sll", d, s1, s2); }
    inline void srl(Register d, Register s1, int s2) { r("srl", d, s1, s2); }
    inline void sub(Register d, Register s1, int s2) { r("sub", d, s1, s2); }
    inline void subw(Register d, Register s1, int s2) { r("subw", d, s1, s2); }
    inline void sra(Register d, Register s1, int s2) { r("sra", d, s1, s2); }
    inline void mul(Register d, Register s1, int s2) { r("mul", d, s1, s2); }
    inline void mulh(Register d, Register s1, int s2) { r("mulh", d, s1, s2); }
    inline void mulhsu(Register d, Register s1, int s2) { r("mulhsu", d, s1, s2); }
    inline void mulhu(Register d, Register s1, int s2) { r("mulhu", d, s1, s2); }
    inline void div(Register d, Register s1, int s2) { r("div", d, s1, s2); }
    inline void divu(Register d, Register s1, int s2) { r("divu", d, s1, s2); }
    inline void rem(Register d, Register s1, int s2) { r("rem", d, s1, s2); }
    inline void remu(Register d, Register s1, int s2) { r("remu", d, s1, s2); }

    inline void nop() { addi(R0, R0, 0); }
    inline void mv(Register d, Register s) { addi(d, s, 0); }
    inline void neg(Register d, Register s) { sub(d, R0, s); }
    inline void negw(Register d, Register s) { subw(d, R0, s); }
    inline void sext_w(Register d, Register s) { addiw(d, s, 0); }
    inline void seqz(Register d, Register s) { sltiu(d, s, 1); }
    inline void snez(Register d, Register s) { sltu(d, R0, s); }
    inline void sltz(Register d, Register s) { slt(d, s, R0); }
    inline void sgtz(Register d, Register s) { slt(d, R0, s); }

    inline void expect(uint64_t value) {
        std::printf("%llu\n", value);
    }

#include "function_being_tested.cpp"

#include "tests.cpp"
};

#undef _
