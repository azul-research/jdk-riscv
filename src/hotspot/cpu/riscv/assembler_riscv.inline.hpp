/*
 * Copyright (c) 2002, 2019, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, 2018 SAP SE. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#ifndef CPU_RISCV_ASSEMBLER_RISCV_INLINE_HPP
#define CPU_RISCV_ASSEMBLER_RISCV_INLINE_HPP

#include "asm/assembler.inline.hpp"
#include "asm/codeBuffer.hpp"
#include "code/codeCache.hpp"

inline void Assembler::emit_int32(int x) {
  AbstractAssembler::emit_int32(x);
}

inline void Assembler::emit_data(int x) {
  emit_int32(x);
}

inline void Assembler::emit_data(int x, relocInfo::relocType rtype) {
  relocate(rtype);
  emit_int32(x);
}

inline void Assembler::emit_data(int x, RelocationHolder const& rspec) {
  relocate(rspec);
  emit_int32(x);
}

// Emit an address
inline address Assembler::emit_addr(const address addr) {
  address start = pc();
  emit_address(addr);
  return start;
}

inline void Assembler::op_imm(   Register d,  Register s, int f, int imm) { emit_int32(OP_IMM_RV_OPCODE | rd(d) | rs1(s) | funct3(f) | immi(imm)); }
inline void Assembler::lui(      Register d,  int imm)                    { emit_int32(LUI_RV_OPCODE | rd(d) | immu(imm)); }
inline void Assembler::auipc(    Register d,  int imm)                    { emit_int32(AUIPC_RV_OPCODE | rd(d) | immu(imm)); }

inline void Assembler::op(       Register d,  Register s1, Register s2, int f1, int f2)                   { emit_int32(OP_RV_OPCODE | rd(d) | rs1(s1) | rs2(s2) | funct3(f1) | funct7(f2)); }
inline void Assembler::amo(      Register d,  Register s1, Register s2, int f1, int f2, bool aq, bool rl) { emit_int32(AMO_RV_OPCODE | rd(d) | rs1(s1) | rs2(s2) | funct3(f1) | funct7(f2, aq, rl)); }

inline void Assembler::jal(      Register d,  int off)                                    { emit_int32(JAL_RV_OPCODE | rd(d) | immj(off)); }
inline void Assembler::jalr(     Register d,  Register base, int off)                     { emit_int32(JALR_RV_OPCODE | rd(d) | rs1(base) | immi(off)); }
inline void Assembler::branch(   Register s1, Register s2,   int f,       int off)        { emit_int32(BRANCH_RV_OPCODE | rs1(s1) | rs2(s2) | funct3(f) | immb(off)); }
inline void Assembler::load(     Register d,  Register s,    int width,   int off)        { emit_int32(LOAD_RV_OPCODE | rd(d) | rs1(s) | funct3(width) | immi(off)); }
inline void Assembler::store(    Register s,  Register base, int width,   int off)        { emit_int32(STORE_RV_OPCODE | rs1(base) | rs2(s) | funct3(width) | imms(off)); }
inline void Assembler::op_imm32( Register d,  Register s,    int f,       int imm)        { emit_int32(OP_IMM32_RV_OPCODE | rd(d) | rs1(s) | funct3(f) | immi(imm)); }
inline void Assembler::op32(     Register d,  Register s1,   Register s2, int f1, int f2) { emit_int32(OP32_RV_OPCODE | rd(d) | rs1(s1) | rs2(s2) | funct3(f1) | funct7(f2)); }

inline void Assembler::load_fp(  FloatRegister d, Register s,    int width, int off) { emit_int32(LOAD_FP_RV_OPCODE | rd(d) | rs1(s) | funct3(width) | immi(off)); }
inline void Assembler::store_fp( FloatRegister s, Register base, int width, int off) { emit_int32(STORE_FP_RV_OPCODE | rs1(base) | rs2(s) | funct3(width) | imms(off)); }

inline void Assembler::op_fp( Register d,      FloatRegister s1, FloatRegister s2, int rm, int f) { emit_int32(OP_FP_RV_OPCODE | rd(d) | rs1(s1) | rs2(s2) | funct3(rm) | funct7(f)); }
inline void Assembler::op_fp( FloatRegister d, FloatRegister s1, FloatRegister s2, int rm, int f) { emit_int32(OP_FP_RV_OPCODE | rd(d) | rs1(s1) | rs2(s2) | funct3(rm) | funct7(f)); }
inline void Assembler::op_fp( FloatRegister d, FloatRegister s1, int s2,           int rm, int f) { emit_int32(OP_FP_RV_OPCODE | rd(d) | rs1(s1) | rs2(s2) | funct3(rm) | funct7(f)); }
inline void Assembler::op_fp( FloatRegister d, Register s1,      int s2,           int rm, int f) { emit_int32(OP_FP_RV_OPCODE | rd(d) | rs1(s1) | rs2(s2) | funct3(rm) | funct7(f)); }
inline void Assembler::op_fp( Register d,      FloatRegister s1, int s2,           int rm, int f) { emit_int32(OP_FP_RV_OPCODE | rd(d) | rs1(s1) | rs2(s2) | funct3(rm) | funct7(f)); }

inline void Assembler::madd(  FloatRegister d, FloatRegister s1, FloatRegister s2, FloatRegister s3, int rm, int f) { emit_int32(MADD_RV_OPCODE  | rd(d) | rs1(s1) | rs2(s2) | rs3(s3) | funct3(rm) | funct2(f)); }
inline void Assembler::msub(  FloatRegister d, FloatRegister s1, FloatRegister s2, FloatRegister s3, int rm, int f) { emit_int32(MSUB_RV_OPCODE  | rd(d) | rs1(s1) | rs2(s2) | rs3(s3) | funct3(rm) | funct2(f)); }
inline void Assembler::nmadd( FloatRegister d, FloatRegister s1, FloatRegister s2, FloatRegister s3, int rm, int f) { emit_int32(NMSUB_RV_OPCODE | rd(d) | rs1(s1) | rs2(s2) | rs3(s3) | funct3(rm) | funct2(f)); }
inline void Assembler::nmsub( FloatRegister d, FloatRegister s1, FloatRegister s2, FloatRegister s3, int rm, int f) { emit_int32(NMADD_RV_OPCODE | rd(d) | rs1(s1) | rs2(s2) | rs3(s3) | funct3(rm) | funct2(f)); }

inline void Assembler::addi(   Register d, Register s, int imm)   { op_imm(d, s, 0x0, imm          ); }
inline void Assembler::slti(   Register d, Register s, int imm)   { op_imm(d, s, 0x2, imm          ); }
inline void Assembler::sltiu(  Register d, Register s, int imm)   { op_imm(d, s, 0x3, imm          ); }
inline void Assembler::xori(   Register d, Register s, int imm)   { op_imm(d, s, 0x4, imm          ); }
inline void Assembler::ori(    Register d, Register s, int imm)   { op_imm(d, s, 0x6, imm          ); }
inline void Assembler::andi(   Register d, Register s, int imm)   { op_imm(d, s, 0x7, imm          ); }
inline void Assembler::slli(   Register d, Register s, int shamt) { op_imm(d, s, 0x1, shamt        ); }
inline void Assembler::srli(   Register d, Register s, int shamt) { op_imm(d, s, 0x5, shamt        ); }
inline void Assembler::srai(   Register d, Register s, int shamt) { op_imm(d, s, 0x5, shamt | 0x400); }

inline void Assembler::add(    Register d, Register s1, Register s2) { op(d, s1, s2, 0x0, 0x0 ); }
inline void Assembler::slt(    Register d, Register s1, Register s2) { op(d, s1, s2, 0x2, 0x0 ); }
inline void Assembler::sltu(   Register d, Register s1, Register s2) { op(d, s1, s2, 0x3, 0x0 ); }
inline void Assembler::andr(   Register d, Register s1, Register s2) { op(d, s1, s2, 0x7, 0x0 ); }
inline void Assembler::orr(    Register d, Register s1, Register s2) { op(d, s1, s2, 0x6, 0x0 ); }
inline void Assembler::xorr(   Register d, Register s1, Register s2) { op(d, s1, s2, 0x4, 0x0 ); }
inline void Assembler::sll(    Register d, Register s1, Register s2) { op(d, s1, s2, 0x1, 0x0 ); }
inline void Assembler::srl(    Register d, Register s1, Register s2) { op(d, s1, s2, 0x5, 0x0 ); }
inline void Assembler::sub(    Register d, Register s1, Register s2) { op(d, s1, s2, 0x0, 0x20); }
inline void Assembler::sra(    Register d, Register s1, Register s2) { op(d, s1, s2, 0x5, 0x20); }
inline void Assembler::mul(    Register d, Register s1, Register s2) { op(d, s1, s2, 0x0, 0x1 ); }
inline void Assembler::mulh(   Register d, Register s1, Register s2) { op(d, s1, s2, 0x1, 0x1 ); }
inline void Assembler::mulhsu( Register d, Register s1, Register s2) { op(d, s1, s2, 0x2, 0x1 ); }
inline void Assembler::mulhu(  Register d, Register s1, Register s2) { op(d, s1, s2, 0x3, 0x1 ); }
inline void Assembler::div(    Register d, Register s1, Register s2) { op(d, s1, s2, 0x4, 0x1 ); }
inline void Assembler::divu(   Register d, Register s1, Register s2) { op(d, s1, s2, 0x5, 0x1 ); }
inline void Assembler::rem(    Register d, Register s1, Register s2) { op(d, s1, s2, 0x6, 0x1 ); }
inline void Assembler::remu(   Register d, Register s1, Register s2) { op(d, s1, s2, 0x7, 0x1 ); }

inline void Assembler::beq(    Register s1, Register s2, int off)  { branch(s1, s2, 0x0, off); }
inline void Assembler::bne(    Register s1, Register s2, int off)  { branch(s1, s2, 0x1, off); }
inline void Assembler::blt(    Register s1, Register s2, int off)  { branch(s1, s2, 0x4, off); }
inline void Assembler::bltu(   Register s1, Register s2, int off)  { branch(s1, s2, 0x6, off); }
inline void Assembler::bge(    Register s1, Register s2, int off)  { branch(s1, s2, 0x5, off); }
inline void Assembler::bgeu(   Register s1, Register s2, int off)  { branch(s1, s2, 0x7, off); }

inline void Assembler::beqz(   Register s, int off)  { beq (s, R0_ZERO, off); }
inline void Assembler::bnez(   Register s, int off)  { bne(s, R0_ZERO, off); }

inline void Assembler::blt(    Register s1, Register s2, Label& L) { blt( s1, s2, disp(intptr_t(target(L)), intptr_t(pc()))); }
inline void Assembler::bge(    Register s1, Register s2, Label& L) { bge( s1, s2, disp(intptr_t(target(L)), intptr_t(pc()))); }
inline void Assembler::beq(    Register s1, Register s2, Label& L) { beq( s1, s2, disp(intptr_t(target(L)), intptr_t(pc()))); }
inline void Assembler::bne(    Register s1, Register s2, Label& L) { bne( s1, s2, disp(intptr_t(target(L)), intptr_t(pc()))); }
inline void Assembler::bgeu(   Register s1, Register s2, Label& L) { bgeu(s1, s2, disp(intptr_t(target(L)), intptr_t(pc()))); }

inline void Assembler::beqz(   Register s, Label& L) { beq(s, R0_ZERO, L); }
inline void Assembler::bnez(   Register s, Label& L) { bne(s, R0_ZERO, L); }

inline void Assembler::ld(     Register d, Register s, int off) { load(d, s, 0x3, off); }
inline void Assembler::lw(     Register d, Register s, int off) { load(d, s, 0x2, off); }
inline void Assembler::lwu(    Register d, Register s, int off) { load(d, s, 0x6, off); }
inline void Assembler::lh(     Register d, Register s, int off) { load(d, s, 0x1, off); }
inline void Assembler::lhu(    Register d, Register s, int off) { load(d, s, 0x5, off); }
inline void Assembler::lb(     Register d, Register s, int off) { load(d, s, 0x0, off); }
inline void Assembler::lbu(    Register d, Register s, int off) { load(d, s, 0x4, off); }

inline void Assembler::sd(Register s, Register base, int off) { store(s, base, 0x3, off); }
inline void Assembler::sw(Register s, Register base, int off) { store(s, base, 0x2, off); }
inline void Assembler::sh(Register s, Register base, int off) { store(s, base, 0x1, off); }
inline void Assembler::sb(Register s, Register base, int off) { store(s, base, 0x0, off); }

inline void Assembler::ecall()  { emit_int32(SYSTEM_RV_OPCODE | immi(0x0)); }
inline void Assembler::ebreak() { emit_int32(SYSTEM_RV_OPCODE | immi(0x1)); }

inline void Assembler::addiw(Register d, Register s, int imm  ) { op_imm32(d, s, 0x0, imm          ); }
inline void Assembler::slliw(Register d, Register s, int shamt) { op_imm32(d, s, 0x1, shamt        ); }
inline void Assembler::srliw(Register d, Register s, int shamt) { op_imm32(d, s, 0x5, shamt        ); }
inline void Assembler::sraiw(Register d, Register s, int shamt) { op_imm32(d, s, 0x5, shamt | 0x400); }

inline void Assembler::addw( Register d, Register s1, Register s2) { op32(d, s1, s2, 0x0, 0x0 ); }
inline void Assembler::subw( Register d, Register s1, Register s2) { op32(d, s1, s2, 0x0, 0x20); }
inline void Assembler::sllw( Register d, Register s1, Register s2) { op32(d, s1, s2, 0x1, 0x0 ); }
inline void Assembler::srlw( Register d, Register s1, Register s2) { op32(d, s1, s2, 0x5, 0x0 ); }
inline void Assembler::sraw( Register d, Register s1, Register s2) { op32(d, s1, s2, 0x5, 0x20); }
inline void Assembler::mulw( Register d, Register s1, Register s2) { op32(d, s1, s2, 0x0, 0x1 ); }
inline void Assembler::divw( Register d, Register s1, Register s2) { op32(d, s1, s2, 0x4, 0x1 ); }
inline void Assembler::divuw(Register d, Register s1, Register s2) { op32(d, s1, s2, 0x5, 0x1 ); }
inline void Assembler::remw( Register d, Register s1, Register s2) { op32(d, s1, s2, 0x6, 0x1 ); }
inline void Assembler::remuw(Register d, Register s1, Register s2) { op32(d, s1, s2, 0x7, 0x1 ); }

inline void Assembler::lrw(     Register d, Register s1,              bool aq, bool rl) { amo(d, s1, 0,  0x2, 0x2,  aq, rl); }
inline void Assembler::scw(     Register d, Register s1, Register s2, bool aq, bool rl) { amo(d, s1, s2, 0x2, 0x3,  aq, rl); }
inline void Assembler::amoswapw(Register d, Register s1, Register s2, bool aq, bool rl) { amo(d, s1, s2, 0x2, 0x1,  aq, rl); }
inline void Assembler::amoaddw( Register d, Register s1, Register s2, bool aq, bool rl) { amo(d, s1, s2, 0x2, 0x0,  aq, rl); }
inline void Assembler::amoxorw( Register d, Register s1, Register s2, bool aq, bool rl) { amo(d, s1, s2, 0x2, 0x4,  aq, rl); }
inline void Assembler::amoandw( Register d, Register s1, Register s2, bool aq, bool rl) { amo(d, s1, s2, 0x2, 0xc,  aq, rl); }
inline void Assembler::amoorw(  Register d, Register s1, Register s2, bool aq, bool rl) { amo(d, s1, s2, 0x2, 0x8,  aq, rl); }
inline void Assembler::amominw( Register d, Register s1, Register s2, bool aq, bool rl) { amo(d, s1, s2, 0x2, 0x10, aq, rl); }
inline void Assembler::amomaxw( Register d, Register s1, Register s2, bool aq, bool rl) { amo(d, s1, s2, 0x2, 0x14, aq, rl); }
inline void Assembler::amominuw(Register d, Register s1, Register s2, bool aq, bool rl) { amo(d, s1, s2, 0x2, 0x18, aq, rl); }
inline void Assembler::amomaxuw(Register d, Register s1, Register s2, bool aq, bool rl) { amo(d, s1, s2, 0x2, 0x1c, aq, rl); }

inline void Assembler::lrd(     Register d, Register s1,              bool aq, bool rl) { amo(d, s1, 0,  0x3, 0x2,  aq, rl); }
inline void Assembler::scd(     Register d, Register s1, Register s2, bool aq, bool rl) { amo(d, s1, s2, 0x3, 0x3,  aq, rl); }
inline void Assembler::amoswapd(Register d, Register s1, Register s2, bool aq, bool rl) { amo(d, s1, s2, 0x3, 0x1,  aq, rl); }
inline void Assembler::amoaddd( Register d, Register s1, Register s2, bool aq, bool rl) { amo(d, s1, s2, 0x3, 0x0,  aq, rl); }
inline void Assembler::amoxord( Register d, Register s1, Register s2, bool aq, bool rl) { amo(d, s1, s2, 0x3, 0x4,  aq, rl); }
inline void Assembler::amoandd( Register d, Register s1, Register s2, bool aq, bool rl) { amo(d, s1, s2, 0x3, 0xc,  aq, rl); }
inline void Assembler::amoord(  Register d, Register s1, Register s2, bool aq, bool rl) { amo(d, s1, s2, 0x3, 0x8,  aq, rl); }
inline void Assembler::amomind( Register d, Register s1, Register s2, bool aq, bool rl) { amo(d, s1, s2, 0x3, 0x10, aq, rl); }
inline void Assembler::amomaxd( Register d, Register s1, Register s2, bool aq, bool rl) { amo(d, s1, s2, 0x3, 0x14, aq, rl); }
inline void Assembler::amominud(Register d, Register s1, Register s2, bool aq, bool rl) { amo(d, s1, s2, 0x3, 0x18, aq, rl); }
inline void Assembler::amomaxud(Register d, Register s1, Register s2, bool aq, bool rl) { amo(d, s1, s2, 0x3, 0x1c, aq, rl); }

inline void Assembler::flw(     FloatRegister d,    Register s, int imm) { load_fp( d,    s, 0x2, imm); }
inline void Assembler::fsw(     FloatRegister s, Register base, int imm) { store_fp(s, base, 0x2, imm); }

inline void Assembler::fmadds(  FloatRegister d, FloatRegister s1, FloatRegister s2, FloatRegister s3, int rm) { madd(d, s1, s2, s3, rm, 0x0); }
inline void Assembler::fmsubs(  FloatRegister d, FloatRegister s1, FloatRegister s2, FloatRegister s3, int rm) { msub(d, s1, s2, s3, rm, 0x0); }
inline void Assembler::fnmadds( FloatRegister d, FloatRegister s1, FloatRegister s2, FloatRegister s3, int rm) { nmadd(d, s1, s2, s3, rm, 0x0); }
inline void Assembler::fnmsubs( FloatRegister d, FloatRegister s1, FloatRegister s2, FloatRegister s3, int rm) { nmsub(d, s1, s2, s3, rm, 0x0); }
inline void Assembler::fadds(   FloatRegister d, FloatRegister s1, FloatRegister s2, int rm) { op_fp(d, s1, s2, rm, 0x0); }
inline void Assembler::fsubs(   FloatRegister d, FloatRegister s1, FloatRegister s2, int rm) { op_fp(d, s1, s2, rm, 0x4); }
inline void Assembler::fmuls(   FloatRegister d, FloatRegister s1, FloatRegister s2, int rm) { op_fp(d, s1, s2, rm, 0x8); }
inline void Assembler::fdivs(   FloatRegister d, FloatRegister s1, FloatRegister s2, int rm) { op_fp(d, s1, s2, rm, 0xc); }
inline void Assembler::fsqrts(  FloatRegister d, FloatRegister s, int rm) { op_fp(d, s, 0x0, rm, 0x2c); }
inline void Assembler::fsgnjs(  FloatRegister d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x0, 0x10); }
inline void Assembler::fsgnjns( FloatRegister d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x1, 0x10); }
inline void Assembler::fsgnjxs( FloatRegister d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x2, 0x10); }
inline void Assembler::fmins(   FloatRegister d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x0, 0x14); }
inline void Assembler::fmaxs(   FloatRegister d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x1, 0x14); }
inline void Assembler::fcvtws(  Register d, FloatRegister s, int rm) { op_fp(d, s, 0x0, rm, 0x60); }
inline void Assembler::fcvtwus( Register d, FloatRegister s, int rm) { op_fp(d, s, 0x1, rm, 0x60); }
inline void Assembler::fmvxw(   Register d, FloatRegister s) { op_fp(d, s, 0x0, 0x0, 0x70); }
inline void Assembler::feqs(    Register d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x2, 0x50); }
inline void Assembler::flts(    Register d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x1, 0x50); }
inline void Assembler::fles(    Register d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x0, 0x50); }
inline void Assembler::fclasss( Register d, FloatRegister s) { op_fp(d, s, 0x0, 0x1, 0x70); }
inline void Assembler::fcvtsw(  FloatRegister d, Register s, int rm) { op_fp(d, s, 0x0, rm, 0x68); }
inline void Assembler::fcvtswu( FloatRegister d, Register s, int rm) { op_fp(d, s, 0x1, rm, 0x68); }
inline void Assembler::fmvwx(   FloatRegister d, Register s) { op_fp(d, s, 0x0, 0x0, 0x78); }
inline void Assembler::fcvtls(  Register d, FloatRegister s, int rm) { op_fp(d, s, 0x2, rm, 0x60); }
inline void Assembler::fcvtlus( Register d, FloatRegister s, int rm) { op_fp(d, s, 0x3, rm, 0x60); }
inline void Assembler::fcvtsl(  FloatRegister d, Register s, int rm) { op_fp(d, s, 0x2, rm, 0x68); }
inline void Assembler::fcvtslu( FloatRegister d, Register s, int rm) { op_fp(d, s, 0x3, rm, 0x68); }

inline void Assembler::fld(     FloatRegister d,    Register s, int imm) { load_fp( d,    s, 0x3, imm); }
inline void Assembler::fsd(     FloatRegister s, Register base, int imm) { store_fp(s, base, 0x3, imm); }

inline void Assembler::fmaddd(  FloatRegister d, FloatRegister s1, FloatRegister s2, FloatRegister s3, int rm) { madd(d, s1, s2, s3, rm, 0x1); }
inline void Assembler::fmsubd(  FloatRegister d, FloatRegister s1, FloatRegister s2, FloatRegister s3, int rm) { msub(d, s1, s2, s3, rm, 0x1); }
inline void Assembler::fnmaddd( FloatRegister d, FloatRegister s1, FloatRegister s2, FloatRegister s3, int rm) { nmadd(d, s1, s2, s3, rm, 0x1); }
inline void Assembler::fnmsubd( FloatRegister d, FloatRegister s1, FloatRegister s2, FloatRegister s3, int rm) { nmsub(d, s1, s2, s3, rm, 0x1); }
inline void Assembler::faddd(   FloatRegister d, FloatRegister s1, FloatRegister s2, int rm) { op_fp(d, s1, s2, rm, 0x1); }
inline void Assembler::fsubd(   FloatRegister d, FloatRegister s1, FloatRegister s2, int rm) { op_fp(d, s1, s2, rm, 0x5); }
inline void Assembler::fmuld(   FloatRegister d, FloatRegister s1, FloatRegister s2, int rm) { op_fp(d, s1, s2, rm, 0x9); }
inline void Assembler::fdivd(   FloatRegister d, FloatRegister s1, FloatRegister s2, int rm) { op_fp(d, s1, s2, rm, 0xd); }
inline void Assembler::fsqrtd(  FloatRegister d, FloatRegister s, int rm) { op_fp(d, s, 0x0, rm, 0x2d); }
inline void Assembler::fsgnjd(  FloatRegister d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x0, 0x11); }
inline void Assembler::fsgnjnd( FloatRegister d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x1, 0x11); }
inline void Assembler::fsgnjxd( FloatRegister d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x2, 0x11); }
inline void Assembler::fmind(   FloatRegister d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x0, 0x15); }
inline void Assembler::fmaxd(   FloatRegister d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x1, 0x15); }
inline void Assembler::fcvtsd(  FloatRegister d, FloatRegister s, int rm) { op_fp(d, s, 0x1, rm, 0x20); }
inline void Assembler::fcvtds(  FloatRegister d, FloatRegister s, int rm) { op_fp(d, s, 0x0, rm, 0x21); }
inline void Assembler::feqd(    Register d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x2, 0x51); }
inline void Assembler::fltd(    Register d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x1, 0x51); }
inline void Assembler::fled(    Register d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x0, 0x51); }
inline void Assembler::fclassd( Register d, FloatRegister s) { op_fp(d, s, 0x0, 0x1, 0x71); }
inline void Assembler::fcvtwd(  Register d, FloatRegister s, int rm) { op_fp(d, s, 0x0, rm, 0x61); }
inline void Assembler::fcvtwud( Register d, FloatRegister s, int rm) { op_fp(d, s, 0x1, rm, 0x61); }
inline void Assembler::fcvtdw(  FloatRegister d, Register s, int rm) { op_fp(d, s, 0x0, rm, 0x69); }
inline void Assembler::fcvtdwu( FloatRegister d, Register s, int rm) { op_fp(d, s, 0x1, rm, 0x69); }
inline void Assembler::fcvtld(  Register d, FloatRegister s, int rm) { op_fp(d, s, 0x2, rm, 0x61); }
inline void Assembler::fcvtlud( Register d, FloatRegister s, int rm) { op_fp(d, s, 0x3, rm, 0x61); }
inline void Assembler::fcvtdl(  FloatRegister d, Register s, int rm) { op_fp(d, s, 0x2, rm, 0x69); }
inline void Assembler::fcvtdlu( FloatRegister d, Register s, int rm) { op_fp(d, s, 0x3, rm, 0x69); }
inline void Assembler::fmvxd(   Register d, FloatRegister s) { op_fp(d, s, 0x0, 0x0, 0x71); }
inline void Assembler::fmvdx(   FloatRegister d, Register s) { op_fp(d, s, 0x0, 0x0, 0x79); }

inline void Assembler::flq(     FloatRegister d,    Register s, int imm) { load_fp( d,    s, 0x4, imm); }
inline void Assembler::fsq(     FloatRegister s, Register base, int imm) { store_fp(s, base, 0x4, imm); }

inline void Assembler::fmaddq(  FloatRegister d, FloatRegister s1, FloatRegister s2, FloatRegister s3, int rm) { madd(d, s1, s2, s3, rm, 0x3); }
inline void Assembler::fmsubq(  FloatRegister d, FloatRegister s1, FloatRegister s2, FloatRegister s3, int rm) { msub(d, s1, s2, s3, rm, 0x3); }
inline void Assembler::fnmaddq( FloatRegister d, FloatRegister s1, FloatRegister s2, FloatRegister s3, int rm) { nmadd(d, s1, s2, s3, rm, 0x3); }
inline void Assembler::fnmsubq( FloatRegister d, FloatRegister s1, FloatRegister s2, FloatRegister s3, int rm) { nmsub(d, s1, s2, s3, rm, 0x3); }
inline void Assembler::faddq(   FloatRegister d, FloatRegister s1, FloatRegister s2, int rm) { op_fp(d, s1, s2, rm, 0x3); }
inline void Assembler::fsubq(   FloatRegister d, FloatRegister s1, FloatRegister s2, int rm) { op_fp(d, s1, s2, rm, 0x7); }
inline void Assembler::fmulq(   FloatRegister d, FloatRegister s1, FloatRegister s2, int rm) { op_fp(d, s1, s2, rm, 0xb); }
inline void Assembler::fdivq(   FloatRegister d, FloatRegister s1, FloatRegister s2, int rm) { op_fp(d, s1, s2, rm, 0xf); }
inline void Assembler::fsqrtq(  FloatRegister d, FloatRegister s, int rm) { op_fp(d, s, 0x0, rm, 0x2f); }
inline void Assembler::fsgnjq(  FloatRegister d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x0, 0x13); }
inline void Assembler::fsgnjnq( FloatRegister d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x1, 0x13); }
inline void Assembler::fsgnjxq( FloatRegister d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x2, 0x13); }
inline void Assembler::fminq(   FloatRegister d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x0, 0x17); }
inline void Assembler::fmaxq(   FloatRegister d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x1, 0x17); }
inline void Assembler::fcvtsq(  FloatRegister d, FloatRegister s, int rm) { op_fp(d, s, 0x3, rm, 0x20); }
inline void Assembler::fcvtqs(  FloatRegister d, FloatRegister s, int rm) { op_fp(d, s, 0x0, rm, 0x23); }
inline void Assembler::fcvtdq(  FloatRegister d, FloatRegister s, int rm) { op_fp(d, s, 0x3, rm, 0x21); }
inline void Assembler::fcvtqd(  FloatRegister d, FloatRegister s, int rm) { op_fp(d, s, 0x1, rm, 0x23); }
inline void Assembler::feqq(    Register d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x2, 0x53); }
inline void Assembler::fltq(    Register d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x1, 0x53); }
inline void Assembler::fleq(    Register d, FloatRegister s1, FloatRegister s2) { op_fp(d, s1, s2, 0x0, 0x53); }
inline void Assembler::fclassq( Register d, FloatRegister s) { op_fp(d, s, 0x0, 0x1, 0x73); }
inline void Assembler::fcvtwq(  Register d, FloatRegister s, int rm) { op_fp(d, s, 0x0, rm, 0x63); }
inline void Assembler::fcvtwuq( Register d, FloatRegister s, int rm) { op_fp(d, s, 0x1, rm, 0x63); }
inline void Assembler::fcvtqw(  FloatRegister d, Register s, int rm) { op_fp(d, s, 0x0, rm, 0x6b); }
inline void Assembler::fcvtqwu( FloatRegister d, Register s, int rm) { op_fp(d, s, 0x1, rm, 0x6b); }
inline void Assembler::fcvtlq(  Register d, FloatRegister s, int rm) { op_fp(d, s, 0x2, rm, 0x63); }
inline void Assembler::fcvtluq( Register d, FloatRegister s, int rm) { op_fp(d, s, 0x3, rm, 0x63); }
inline void Assembler::fcvtql(  FloatRegister d, Register s, int rm) { op_fp(d, s, 0x2, rm, 0x6b); }
inline void Assembler::fcvtqlu( FloatRegister d, Register s, int rm) { op_fp(d, s, 0x3, rm, 0x6b); }
// pseudoinstructions
inline void Assembler::nop() { addi(R0, R0, 0); }
inline void Assembler::j(int off) { jal(R0, off); }
inline void Assembler::jal(int off) { jal(R1, off); }
inline void Assembler::jr(Register s) { jalr(R0, s, 0); }
inline void Assembler::jalr(Register s) { jalr(R1, s, 0); }
inline void Assembler::ret() { jr(R1); }
inline void Assembler::call(int off) { auipc(R1, off & 0xfffff000); jalr(R1, R1, off & 0xfff); }
inline void Assembler::tail(int off) { auipc(R6, off & 0xfffff000); jalr(R0, R6, off & 0xfff); }
inline void Assembler::neg(Register d, Register s) { sub(d, R0, s); }
inline void Assembler::negw(Register d, Register s) { subw(d, R0, s); }
inline void Assembler::mv(Register d, Register s) { addi(d, s, 0); }
inline void Assembler::fnegs(FloatRegister rd, FloatRegister rs) { fsgnjns(rd, rs, rs); }
inline void Assembler::fnegd(FloatRegister rd, FloatRegister rs) { fsgnjnd(rd, rs, rs); }

inline void Assembler::j(Label& L) { j(disp(intptr_t(target(L)), intptr_t(pc()))); }

// --- PPC instructions follow ---

// Issue an illegal instruction. 0 is guaranteed to be an illegal instruction.
inline void Assembler::illtrap() { Assembler::emit_int32(0); } // FIXME_RISCV set more weird number
inline bool Assembler::is_illtrap(int x) { return x == 0; }

// RISCV 1, section 3.3.8, Fixed-Point Arithmetic Instructions
inline void Assembler::addi_PPC(   Register d, Register a, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addis_PPC(  Register d, Register a, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addi_r0ok_PPC(Register d,Register a,int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addis_r0ok_PPC(Register d,Register a,int si16)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addic__PPC( Register d, Register a, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfic_PPC( Register d, Register a, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::add_PPC(    Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::add__PPC(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subf_PPC(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sub_PPC(    Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subf__PPC(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addc_PPC(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addc__PPC(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfc_PPC(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfc__PPC( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::adde_PPC(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::adde__PPC(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfe_PPC(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfe__PPC( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addme_PPC(  Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addme__PPC( Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfme_PPC( Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfme__PPC(Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addze_PPC(  Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addze__PPC( Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfze_PPC( Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfze__PPC(Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::neg_PPC(    Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::neg__PPC(   Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulli_PPC(  Register d, Register a, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulld_PPC(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulld__PPC( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mullw_PPC(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mullw__PPC( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulhw_PPC(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulhw__PPC( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulhwu_PPC( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulhwu__PPC(Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulhd_PPC(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulhd__PPC( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulhdu_PPC( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulhdu__PPC(Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::divd_PPC(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::divd__PPC(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::divw_PPC(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::divw__PPC(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed

// Fixed-Point Arithmetic Instructions with Overflow detection
inline void Assembler::addo_PPC(    Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addo__PPC(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfo_PPC(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfo__PPC(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addco_PPC(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addco__PPC(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfco_PPC(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfco__PPC( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addeo_PPC(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addeo__PPC(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfeo_PPC(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfeo__PPC( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addmeo_PPC(  Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addmeo__PPC( Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfmeo_PPC( Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfmeo__PPC(Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addzeo_PPC(  Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addzeo__PPC( Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfzeo_PPC( Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfzeo__PPC(Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::nego_PPC(    Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::nego__PPC(   Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulldo_PPC(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulldo__PPC( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mullwo_PPC(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mullwo__PPC( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::divdo_PPC(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::divdo__PPC(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::divwo_PPC(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::divwo__PPC(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed

// extended mnemonics
inline void Assembler::li_PPC(   Register d, int si16)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lis_PPC(  Register d, int si16)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addir_PPC(Register d, int si16, Register a) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subi_PPC( Register d, Register a, int si16) { illtrap(); } // This is a PPC instruction and should be removed

// RISCV 1, section 3.3.9, Fixed-Point Compare Instructions
inline void Assembler::cmpi_PPC(  ConditionRegister f, int l, Register a, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmp_PPC(   ConditionRegister f, int l, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmpli_PPC( ConditionRegister f, int l, Register a, int ui16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmpl_PPC(  ConditionRegister f, int l, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmprb_PPC( ConditionRegister f, int l, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmpeqb_PPC(ConditionRegister f, Register a, Register b)        { illtrap(); } // This is a PPC instruction and should be removed

// extended mnemonics of Compare Instructions
inline void Assembler::cmpwi_PPC( ConditionRegister crx, Register a, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmpdi_PPC( ConditionRegister crx, Register a, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmpw_PPC(  ConditionRegister crx, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmpd_PPC(  ConditionRegister crx, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmplwi_PPC(ConditionRegister crx, Register a, int ui16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmpldi_PPC(ConditionRegister crx, Register a, int ui16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmplw_PPC( ConditionRegister crx, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmpld_PPC( ConditionRegister crx, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::isel_PPC(Register d, Register a, Register b, int c) { illtrap(); } // This is a PPC instruction and should be removed

// RISCV 1, section 3.3.11, Fixed-Point Logical Instructions
inline void Assembler::andi__PPC(   Register a, Register s, int ui16)      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::andis__PPC(  Register a, Register s, int ui16)      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ori_PPC(     Register a, Register s, int ui16)      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::oris_PPC(    Register a, Register s, int ui16)      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xori_PPC(    Register a, Register s, int ui16)      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xoris_PPC(   Register a, Register s, int ui16)      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::andr_PPC(    Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::and__PPC(    Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::or_unchecked_PPC(Register a, Register s, Register b){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::orr_PPC(     Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::or__PPC(     Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xorr_PPC(    Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xor__PPC(    Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::nand_PPC(    Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::nand__PPC(   Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::nor_PPC(     Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::nor__PPC(    Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::andc_PPC(    Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::andc__PPC(   Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::orc_PPC(     Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::orc__PPC(    Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::extsb_PPC(   Register a, Register s)                { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::extsb__PPC(  Register a, Register s)                { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::extsh_PPC(   Register a, Register s)                { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::extsh__PPC(  Register a, Register s)                { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::extsw_PPC(   Register a, Register s)                { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::extsw__PPC(  Register a, Register s)                { illtrap(); } // This is a PPC instruction and should be removed

// extended mnemonics
inline void Assembler::nop_PPC()                              { illtrap(); } // This is a PPC instruction and should be removed
// NOP for FP and BR units (different versions to allow them to be in one group)
inline void Assembler::fpnop0_PPC()                           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fpnop1_PPC()                           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::brnop0_PPC()                           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::brnop1_PPC()                           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::brnop2_PPC()                           { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::mr_PPC(      Register d, Register s)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ori_opt_PPC( Register d, int ui16)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::oris_opt_PPC(Register d, int ui16)     { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::endgroup_PPC()                         { illtrap(); } // This is a PPC instruction and should be removed

// count instructions
inline void Assembler::cntlzw_PPC(  Register a, Register s)              { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cntlzw__PPC( Register a, Register s)              { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cntlzd_PPC(  Register a, Register s)              { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cntlzd__PPC( Register a, Register s)              { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cnttzw_PPC(  Register a, Register s)              { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cnttzw__PPC( Register a, Register s)              { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cnttzd_PPC(  Register a, Register s)              { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cnttzd__PPC( Register a, Register s)              { illtrap(); } // This is a PPC instruction and should be removed

// RISCV 1, section 3.3.12, Fixed-Point Rotate and Shift Instructions
inline void Assembler::sld_PPC(     Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sld__PPC(    Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::slw_PPC(     Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::slw__PPC(    Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srd_PPC(     Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srd__PPC(    Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srw_PPC(     Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srw__PPC(    Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srad_PPC(    Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srad__PPC(   Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sraw_PPC(    Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sraw__PPC(   Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sradi_PPC(   Register a, Register s, int sh6)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sradi__PPC(  Register a, Register s, int sh6)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srawi_PPC(   Register a, Register s, int sh5)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srawi__PPC(  Register a, Register s, int sh5)     { illtrap(); } // This is a PPC instruction and should be removed

// extended mnemonics for Shift Instructions
inline void Assembler::sldi_PPC(    Register a, Register s, int sh6)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sldi__PPC(   Register a, Register s, int sh6)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::slwi_PPC(    Register a, Register s, int sh5)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::slwi__PPC(   Register a, Register s, int sh5)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srdi_PPC(    Register a, Register s, int sh6)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srdi__PPC(   Register a, Register s, int sh6)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srwi_PPC(    Register a, Register s, int sh5)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srwi__PPC(   Register a, Register s, int sh5)     { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::clrrdi_PPC(  Register a, Register s, int ui6)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::clrrdi__PPC( Register a, Register s, int ui6)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::clrldi_PPC(  Register a, Register s, int ui6)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::clrldi__PPC( Register a, Register s, int ui6)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::clrlsldi_PPC( Register a, Register s, int clrl6, int shl6) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::clrlsldi__PPC(Register a, Register s, int clrl6, int shl6) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::extrdi_PPC(  Register a, Register s, int n, int b){ illtrap(); } // This is a PPC instruction and should be removed
// testbit with condition register.
inline void Assembler::testbitdi_PPC(ConditionRegister cr, Register a, Register s, int ui6) { illtrap(); } // This is a PPC instruction and should be removed

// rotate instructions
inline void Assembler::rotldi_PPC( Register a, Register s, int n) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rotrdi_PPC( Register a, Register s, int n) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rotlwi_PPC( Register a, Register s, int n) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rotrwi_PPC( Register a, Register s, int n) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::rldic_PPC(   Register a, Register s, int sh6, int mb6)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rldic__PPC(  Register a, Register s, int sh6, int mb6)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rldicr_PPC(  Register a, Register s, int sh6, int mb6)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rldicr__PPC( Register a, Register s, int sh6, int mb6)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rldicl_PPC(  Register a, Register s, int sh6, int me6)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rldicl__PPC( Register a, Register s, int sh6, int me6)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rlwinm_PPC(  Register a, Register s, int sh5, int mb5, int me5){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rlwinm__PPC( Register a, Register s, int sh5, int mb5, int me5){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rldimi_PPC(  Register a, Register s, int sh6, int mb6)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rlwimi_PPC(  Register a, Register s, int sh5, int mb5, int me5){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rldimi__PPC( Register a, Register s, int sh6, int mb6)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::insrdi_PPC(  Register a, Register s, int n,   int b)           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::insrwi_PPC(  Register a, Register s, int n,   int b)           { illtrap(); } // This is a PPC instruction and should be removed

// RISCV 1, section 3.3.2 Fixed-Point Load Instructions
inline void Assembler::lwzx_PPC( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwz_PPC(  Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwzu_PPC( Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::lwax_PPC( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwa_PPC(  Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::lwbrx_PPC( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::lhzx_PPC( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lhz_PPC(  Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lhzu_PPC( Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::lhbrx_PPC( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::lhax_PPC( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lha_PPC(  Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lhau_PPC( Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::lbzx_PPC( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lbz_PPC(  Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lbzu_PPC( Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::ld_PPC(   Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ldx_PPC(  Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ldu_PPC(  Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ldbrx_PPC( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::ld_ptr_PPC(Register d, int b, Register s1) { illtrap(); } // This is a PPC instruction and should be removed
DEBUG_ONLY(inline void Assembler::ld_ptr_PPC(Register d, ByteSize b, Register s1) { illtrap(); }) // This is a PPC instruction and should be removed

//  RISCV 1, section 3.3.3 Fixed-Point Store Instructions
inline void Assembler::stwx_PPC( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stw_PPC(  Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stwu_PPC( Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stwbrx_PPC( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::sthx_PPC( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sth_PPC(  Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sthu_PPC( Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sthbrx_PPC( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::stbx_PPC( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stb_PPC(  Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stbu_PPC( Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::std_PPC(  Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stdx_PPC( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stdu_PPC( Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stdux_PPC(Register s, Register a,  Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stdbrx_PPC( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::st_ptr_PPC(Register d, int b, Register s1) { illtrap(); } // This is a PPC instruction and should be removed
DEBUG_ONLY(inline void Assembler::st_ptr_PPC(Register d, ByteSize b, Register s1) { illtrap(); }) // This is a PPC instruction and should be removed

// RISCV 1, section 3.3.13 Move To/From System Register Instructions
inline void Assembler::mtlr_PPC( Register s1)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mflr_PPC( Register d )         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mtctr_PPC(Register s1)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mfctr_PPC(Register d )         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mtcrf_PPC(int afxm, Register s){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mfcr_PPC( Register d )         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mcrf_PPC( ConditionRegister crd, ConditionRegister cra)
                                                      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mtcr_PPC( Register s)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::setb_PPC(Register d, ConditionRegister cra)
                                                  { illtrap(); } // This is a PPC instruction and should be removed

// Special purpose registers
// Exception Register
inline void Assembler::mtxer_PPC(Register s1)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mfxer_PPC(Register d )         { illtrap(); } // This is a PPC instruction and should be removed
// Vector Register Save Register
inline void Assembler::mtvrsave_PPC(Register s1)      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mfvrsave_PPC(Register d )      { illtrap(); } // This is a PPC instruction and should be removed
// Timebase
inline void Assembler::mftb_PPC(Register d )          { illtrap(); } // This is a PPC instruction and should be removed
// Introduced with Power 8:
// Data Stream Control Register
inline void Assembler::mtdscr_PPC(Register s1)        { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mfdscr_PPC(Register d )        { illtrap(); } // This is a PPC instruction and should be removed
// Transactional Memory Registers
inline void Assembler::mftfhar_PPC(Register d )       { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mftfiar_PPC(Register d )       { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mftexasr_PPC(Register d )      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mftexasru_PPC(Register d )     { illtrap(); } // This is a PPC instruction and should be removed

// SAP JVM 2006-02-13 RISCV branch instruction.
// RISCV 1, section 2.4.1 Branch Instructions
inline void Assembler::b_PPC( address a, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::b_PPC( Label& L)                           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bl_PPC(address a, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bl_PPC(Label& L)                           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bc_PPC( int boint, int biint, address a, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bc_PPC( int boint, int biint, Label& L)                           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bcl_PPC(int boint, int biint, address a, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bcl_PPC(int boint, int biint, Label& L)                           { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::bclr_PPC(  int boint, int biint, int bhint, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bclrl_PPC( int boint, int biint, int bhint, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bcctr_PPC( int boint, int biint, int bhint, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bcctrl_PPC(int boint, int biint, int bhint, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed

// helper function for b
inline bool Assembler::is_within_range_of_b(address a, address pc) {
  // Guard against illegal branch targets, e.g. -1 (see CompiledStaticCall and ad-file).
  if ((((uint64_t)a) & 0x3) != 0) return false;

  const int range = 1 << (29-6); // li field is from bit 6 to bit 29.
  int value = disp(intptr_t(a), intptr_t(pc));
  bool result = -range <= value && value < range-1;
#ifdef ASSERT
  if (result) li_PPC(value); // Assert that value is in correct range.
#endif
  return result;
}

// helper functions for bcxx.
inline bool Assembler::is_within_range_of_bcxx(address a, address pc) {
  // Guard against illegal branch targets, e.g. -1 (see CompiledStaticCall and ad-file).
  if ((((uint64_t)a) & 0x3) != 0) return false;

  const int range = 1 << (29-16); // bd field is from bit 16 to bit 29.
  int value = disp(intptr_t(a), intptr_t(pc));
  bool result = -range <= value && value < range-1;
#ifdef ASSERT
  if (result) bd(value); // Assert that value is in correct range.
#endif
  return result;
}

// Get the destination of a bxx branch (b, bl, ba, bla).
address  Assembler::bxx_destination_PPC(address baddr) { return bxx_destination_PPC(*(int*)baddr, baddr); }
address  Assembler::bxx_destination_PPC(int instr, address pc) { return (address)bxx_destination_offset(instr, (intptr_t)pc); }
intptr_t Assembler::bxx_destination_offset(int instr, intptr_t bxx_pos) {
  intptr_t displ = inv_li_field(instr);
  return bxx_pos + displ;
}

// Extended mnemonics for Branch Instructions
inline void Assembler::blt_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bgt_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::beq_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bso_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bge_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ble_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bne_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bns_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed

// Branch instructions with static prediction hints.
inline void Assembler::blt_predict_taken_PPC    (ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bgt_predict_taken_PPC    (ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::beq_predict_taken_PPC    (ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bso_predict_taken_PPC    (ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bge_predict_taken_PPC    (ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ble_predict_taken_PPC    (ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bne_predict_taken_PPC    (ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bns_predict_taken_PPC    (ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::blt_predict_not_taken_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bgt_predict_not_taken_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::beq_predict_not_taken_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bso_predict_not_taken_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bge_predict_not_taken_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ble_predict_not_taken_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bne_predict_not_taken_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bns_predict_not_taken_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed

// For use in conjunction with testbitdi:
inline void Assembler::btrue_PPC( ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bfalse_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::bltl_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bgtl_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::beql_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bsol_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bgel_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::blel_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bnel_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bnsl_PPC(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed

// Extended mnemonics for Branch Instructions via LR.
// We use `blr' for returns.
inline void Assembler::blr_PPC(relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed

// Extended mnemonics for Branch Instructions with CTR.
// Bdnz means `decrement CTR and jump to L if CTR is not zero'.
inline void Assembler::bdnz_PPC(Label& L) { illtrap(); } // This is a PPC instruction and should be removed
// Decrement and branch if result is zero.
inline void Assembler::bdz_PPC(Label& L)  { illtrap(); } // This is a PPC instruction and should be removed
// We use `bctr[l]' for jumps/calls in function descriptor glue
// code, e.g. for calls to runtime functions.
inline void Assembler::bctr_PPC( relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bctrl_PPC(relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
// Conditional jumps/branches via CTR.
inline void Assembler::beqctr_PPC( ConditionRegister crx, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::beqctrl_PPC(ConditionRegister crx, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bnectr_PPC( ConditionRegister crx, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bnectrl_PPC(ConditionRegister crx, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed

// condition register logic instructions
inline void Assembler::crand_PPC( int d, int s1, int s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::crnand_PPC(int d, int s1, int s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cror_PPC(  int d, int s1, int s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::crxor_PPC( int d, int s1, int s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::crnor_PPC( int d, int s1, int s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::creqv_PPC( int d, int s1, int s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::crandc_PPC(int d, int s1, int s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::crorc_PPC( int d, int s1, int s2) { illtrap(); } // This is a PPC instruction and should be removed

// More convenient version.
inline void Assembler::crand_PPC( ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc) {
  int dst_bit = condition_register_bit(crdst, cdst),
      src_bit = condition_register_bit(crsrc, csrc);
  crand_PPC(dst_bit, src_bit, dst_bit);
}
inline void Assembler::crnand_PPC(ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc) {
  int dst_bit = condition_register_bit(crdst, cdst),
      src_bit = condition_register_bit(crsrc, csrc);
  crnand_PPC(dst_bit, src_bit, dst_bit);
}
inline void Assembler::cror_PPC(  ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc) {
  int dst_bit = condition_register_bit(crdst, cdst),
      src_bit = condition_register_bit(crsrc, csrc);
  cror_PPC(dst_bit, src_bit, dst_bit);
}
inline void Assembler::crxor_PPC( ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc) {
  int dst_bit = condition_register_bit(crdst, cdst),
      src_bit = condition_register_bit(crsrc, csrc);
  crxor_PPC(dst_bit, src_bit, dst_bit);
}
inline void Assembler::crnor_PPC( ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc) {
  int dst_bit = condition_register_bit(crdst, cdst),
      src_bit = condition_register_bit(crsrc, csrc);
  crnor_PPC(dst_bit, src_bit, dst_bit);
}
inline void Assembler::creqv_PPC( ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc) {
  int dst_bit = condition_register_bit(crdst, cdst),
      src_bit = condition_register_bit(crsrc, csrc);
  creqv_PPC(dst_bit, src_bit, dst_bit);
}
inline void Assembler::crandc_PPC(ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc) {
  int dst_bit = condition_register_bit(crdst, cdst),
      src_bit = condition_register_bit(crsrc, csrc);
  crandc_PPC(dst_bit, src_bit, dst_bit);
}
inline void Assembler::crorc_PPC( ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc) {
  int dst_bit = condition_register_bit(crdst, cdst),
      src_bit = condition_register_bit(crsrc, csrc);
  crorc_PPC(dst_bit, src_bit, dst_bit);
}

// Conditional move (>= Power7)
inline void Assembler::isel_PPC(Register d, ConditionRegister cr, Condition cc, bool inv, Register a, Register b) {
  if (b == noreg) {
    b = d; // Can be omitted if old value should be kept in "else" case.
  }
  Register first = a;
  Register second = b;
  if (inv) {
    first = b;
    second = a; // exchange
  }
  assert(first != R0, "r0 not allowed");
  isel_PPC(d, first, second, bi0(cr, cc));
}
inline void Assembler::isel_0_PPC(Register d, ConditionRegister cr, Condition cc, Register b) {
  if (b == noreg) {
    b = d; // Can be omitted if old value should be kept in "else" case.
  }
  isel_PPC(d, R0, b, bi0(cr, cc));
}

// RISCV 2, section 3.2.1 Instruction Cache Instructions
inline void Assembler::icbi_PPC(    Register s1, Register s2)         { illtrap(); } // This is a PPC instruction and should be removed
// RISCV 2, section 3.2.2 Data Cache Instructions
//inline void Assembler::dcba(  Register s1, Register s2)         { emit_int32_PPC( DCBA_OPCODE   | ra0mem(s1) | rb(s2)           ); }
inline void Assembler::dcbz_PPC(    Register s1, Register s2)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbst_PPC(   Register s1, Register s2)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbf_PPC(    Register s1, Register s2)         { illtrap(); } // This is a PPC instruction and should be removed
// dcache read hint
inline void Assembler::dcbt_PPC(    Register s1, Register s2)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbtct_PPC(  Register s1, Register s2, int ct) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbtds_PPC(  Register s1, Register s2, int ds) { illtrap(); } // This is a PPC instruction and should be removed
// dcache write hint
inline void Assembler::dcbtst_PPC(  Register s1, Register s2)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbtstct_PPC(Register s1, Register s2, int ct) { illtrap(); } // This is a PPC instruction and should be removed

// machine barrier instructions:
inline void Assembler::sync_PPC(int a) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sync_PPC()      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwsync_PPC()    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ptesync_PPC()   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::eieio_PPC()     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::isync_PPC()     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::elemental_membar_PPC(int e) { illtrap(); } // This is a PPC instruction and should be removed

// Wait instructions for polling.
inline void Assembler::wait_PPC()    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::waitrsv_PPC() { illtrap(); } // This is a PPC instruction and should be removed // WC=0b01 >=Power7

// atomics
// Use ra0mem to disallow R0 as base.
inline void Assembler::lbarx_unchecked_PPC(Register d, Register a, Register b, int eh1)           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lharx_unchecked_PPC(Register d, Register a, Register b, int eh1)           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwarx_unchecked_PPC(Register d, Register a, Register b, int eh1)           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ldarx_unchecked_PPC(Register d, Register a, Register b, int eh1)           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lqarx_unchecked_PPC(Register d, Register a, Register b, int eh1)           { illtrap(); } // This is a PPC instruction and should be removed
inline bool Assembler::lxarx_hint_exclusive_access_PPC()                                          { return VM_Version::has_lxarxeh(); }
inline void Assembler::lbarx_PPC( Register d, Register a, Register b, bool hint_exclusive_access) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lharx_PPC( Register d, Register a, Register b, bool hint_exclusive_access) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwarx_PPC( Register d, Register a, Register b, bool hint_exclusive_access) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ldarx_PPC( Register d, Register a, Register b, bool hint_exclusive_access) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lqarx_PPC( Register d, Register a, Register b, bool hint_exclusive_access) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stbcx__PPC(Register s, Register a, Register b)                             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sthcx__PPC(Register s, Register a, Register b)                             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stwcx__PPC(Register s, Register a, Register b)                             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stdcx__PPC(Register s, Register a, Register b)                             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stqcx__PPC(Register s, Register a, Register b)                             { illtrap(); } // This is a PPC instruction and should be removed

// Instructions for adjusting thread priority
// for simultaneous multithreading (SMT) on >= POWER5.
inline void Assembler::smt_prio_very_low_PPC()    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::smt_prio_low_PPC()         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::smt_prio_medium_low_PPC()  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::smt_prio_medium_PPC()      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::smt_prio_medium_high_PPC() { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::smt_prio_high_PPC()        { illtrap(); } // This is a PPC instruction and should be removed
// >= Power7
inline void Assembler::smt_yield_PPC()            { illtrap(); } // This is a PPC instruction and should be removed // never actually implemented
inline void Assembler::smt_mdoio_PPC()            { illtrap(); } // This is a PPC instruction and should be removed // never actually implemetned
inline void Assembler::smt_mdoom_PPC()            { illtrap(); } // This is a PPC instruction and should be removed // never actually implemented
// Power8
inline void Assembler::smt_miso_PPC()             { illtrap(); } // This is a PPC instruction and should be removed // never actually implemented

inline void Assembler::twi_0_PPC(Register a)      { illtrap(); } // This is a PPC instruction and should be removed

// trap instructions
inline void Assembler::tdi_unchecked_PPC(int tobits, Register a, int si16){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::twi_unchecked_PPC(int tobits, Register a, int si16){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tdi_PPC(int tobits, Register a, int si16)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::twi_PPC(int tobits, Register a, int si16)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::td_PPC( int tobits, Register a, Register b)        { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tw_PPC( int tobits, Register a, Register b)        { illtrap(); } // This is a PPC instruction and should be removed

// FLOATING POINT instructions riscv.
// RISCV 1, section 4.6.2 Floating-Point Load Instructions
// Use ra0mem instead of ra in some instructions below.
inline void Assembler::lfs_PPC( FloatRegister d, int si16, Register a)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lfsu_PPC(FloatRegister d, int si16, Register a)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lfsx_PPC(FloatRegister d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lfd_PPC( FloatRegister d, int si16, Register a)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lfdu_PPC(FloatRegister d, int si16, Register a)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lfdx_PPC(FloatRegister d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed

// RISCV 1, section 4.6.3 Floating-Point Store Instructions
// Use ra0mem instead of ra in some instructions below.
inline void Assembler::stfs_PPC( FloatRegister s, int si16, Register a)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stfsu_PPC(FloatRegister s, int si16, Register a)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stfsx_PPC(FloatRegister s, Register a, Register b){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stfd_PPC( FloatRegister s, int si16, Register a)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stfdu_PPC(FloatRegister s, int si16, Register a)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stfdx_PPC(FloatRegister s, Register a, Register b){ illtrap(); } // This is a PPC instruction and should be removed

// RISCV 1, section 4.6.4 Floating-Point Move Instructions
inline void Assembler::fmr_PPC( FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmr__PPC(FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed

// These are special Power6 opcodes, reused for "lfdepx" and "stfdepx"
// on Power7.  Do not use.
//inline void Assembler::mffgpr( FloatRegister d, Register b)   { emit_int32_PPC( MFFGPR_OPCODE | frt(d) | rb(b) | rc(0)); }
//inline void Assembler::mftgpr( Register d, FloatRegister b)   { emit_int32_PPC( MFTGPR_OPCODE | rt(d) | frb(b) | rc(0)); }
// add cmpb and popcntb to detect riscv power version.
inline void Assembler::cmpb_PPC(   Register a, Register s, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::popcntb_PPC(Register a, Register s)             { illtrap(); } // This is a PPC instruction and should be removed;
inline void Assembler::popcntw_PPC(Register a, Register s)             { illtrap(); } // This is a PPC instruction and should be removed;
inline void Assembler::popcntd_PPC(Register a, Register s)             { illtrap(); } // This is a PPC instruction and should be removed;

inline void Assembler::fneg_PPC(  FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fneg__PPC( FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fabs_PPC(  FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fabs__PPC( FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fnabs_PPC( FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fnabs__PPC(FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed

// RISCV 1, section 4.6.5.1 Floating-Point Elementary Arithmetic Instructions
inline void Assembler::fadd_PPC(  FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fadd__PPC( FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fadds_PPC( FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fadds__PPC(FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fsub_PPC(  FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fsub__PPC( FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fsubs_PPC( FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fsubs__PPC(FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmul_PPC(  FloatRegister d, FloatRegister a, FloatRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmul__PPC( FloatRegister d, FloatRegister a, FloatRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmuls_PPC( FloatRegister d, FloatRegister a, FloatRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmuls__PPC(FloatRegister d, FloatRegister a, FloatRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fdiv_PPC(  FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fdiv__PPC( FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fdivs_PPC( FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fdivs__PPC(FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed

// Fused multiply-accumulate instructions.
// WARNING: Use only when rounding between the 2 parts is not desired.
// Some floating point tck tests will fail if used incorrectly.
inline void Assembler::fmadd_PPC(   FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmadd__PPC(  FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmadds_PPC(  FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmadds__PPC( FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmsub_PPC(   FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmsub__PPC(  FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmsubs_PPC(  FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmsubs__PPC( FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fnmadd_PPC(  FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fnmadd__PPC( FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fnmadds_PPC( FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fnmadds__PPC(FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fnmsub_PPC(  FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fnmsub__PPC( FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fnmsubs_PPC( FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fnmsubs__PPC(FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed

// RISCV 1, section 4.6.6 Floating-Point Rounding and Conversion Instructions
inline void Assembler::frsp_PPC(  FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fctid_PPC( FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fctidz_PPC(FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fctiw_PPC( FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fctiwz_PPC(FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fcfid_PPC( FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fcfids_PPC(FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed

// RISCV 1, section 4.6.7 Floating-Point Compare Instructions
inline void Assembler::fcmpu_PPC( ConditionRegister crx, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed

// RISCV 1, section 5.2.1 Floating-Point Arithmetic Instructions
inline void Assembler::fsqrt_PPC( FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fsqrts_PPC(FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed

// Vector instructions for >= Power6.
inline void Assembler::lvebx_PPC( VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvehx_PPC( VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvewx_PPC( VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvx_PPC(   VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvxl_PPC(  VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stvebx_PPC(VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stvehx_PPC(VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stvewx_PPC(VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stvx_PPC(  VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stvxl_PPC( VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvsl_PPC(  VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvsr_PPC(  VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed

// Vector-Scalar (VSX) instructions.
inline void Assembler::lxvd2x_PPC(  VectorSRegister d, Register s1)              { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lxvd2x_PPC(  VectorSRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stxvd2x_PPC( VectorSRegister d, Register s1)              { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stxvd2x_PPC( VectorSRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mtvsrd_PPC(  VectorSRegister d, Register a)               { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mtvsrwz_PPC( VectorSRegister d, Register a)               { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xxspltw_PPC( VectorSRegister d, VectorSRegister b, int ui2)           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xxlor_PPC(   VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xxlxor_PPC(  VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xxleqv_PPC(  VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvdivsp_PPC( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvdivdp_PPC( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvabssp_PPC( VectorSRegister d, VectorSRegister b)                    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvabsdp_PPC( VectorSRegister d, VectorSRegister b)                    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvnegsp_PPC( VectorSRegister d, VectorSRegister b)                    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvnegdp_PPC( VectorSRegister d, VectorSRegister b)                    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvsqrtsp_PPC(VectorSRegister d, VectorSRegister b)                    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvsqrtdp_PPC(VectorSRegister d, VectorSRegister b)                    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xscvdpspn_PPC(VectorSRegister d, VectorSRegister b)                   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvadddp_PPC( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvsubdp_PPC( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvmulsp_PPC( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvmuldp_PPC( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvmaddasp_PPC( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvmaddadp_PPC( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvmsubasp_PPC( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvmsubadp_PPC( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvnmsubasp_PPC(VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvnmsubadp_PPC(VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mtvrd_PPC(   VectorRegister d, Register a)               { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mfvrd_PPC(   Register        a, VectorRegister d)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mtvrwz_PPC(  VectorRegister  d, Register a)               { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mfvrwz_PPC(  Register        a, VectorRegister d)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xxpermdi_PPC(VectorSRegister d, VectorSRegister a, VectorSRegister b, int dm) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xxmrghw_PPC( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xxmrglw_PPC( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed

// VSX Extended Mnemonics
inline void Assembler::xxspltd_PPC( VectorSRegister d, VectorSRegister a, int x)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xxmrghd_PPC( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xxmrgld_PPC( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xxswapd_PPC( VectorSRegister d, VectorSRegister a)                    { illtrap(); } // This is a PPC instruction and should be removed

// Vector-Scalar (VSX) instructions.
inline void Assembler::mtfprd_PPC(  FloatRegister   d, Register a)      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mtfprwa_PPC( FloatRegister   d, Register a)      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mffprd_PPC(  Register        a, FloatRegister d) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::vpkpx_PPC(   VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpkshss_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpkswss_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpkshus_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpkswus_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpkuhum_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpkuwum_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpkuhus_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpkuwus_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vupkhpx_PPC( VectorRegister d, VectorRegister b)                   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vupkhsb_PPC( VectorRegister d, VectorRegister b)                   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vupkhsh_PPC( VectorRegister d, VectorRegister b)                   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vupklpx_PPC( VectorRegister d, VectorRegister b)                   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vupklsb_PPC( VectorRegister d, VectorRegister b)                   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vupklsh_PPC( VectorRegister d, VectorRegister b)                   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmrghb_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmrghw_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmrghh_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmrglb_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmrglw_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmrglh_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsplt_PPC(   VectorRegister d, int ui4,          VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsplth_PPC(  VectorRegister d, int ui3,          VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vspltw_PPC(  VectorRegister d, int ui2,          VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vspltisb_PPC(VectorRegister d, int si5)                            { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vspltish_PPC(VectorRegister d, int si5)                            { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vspltisw_PPC(VectorRegister d, int si5)                            { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vperm_PPC(   VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsel_PPC(    VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsl_PPC(     VectorRegister d, VectorRegister a, VectorRegister b)                  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsldoi_PPC(  VectorRegister d, VectorRegister a, VectorRegister b, int ui4)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vslo_PPC(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsr_PPC(     VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsro_PPC(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vaddcuw_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vaddshs_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vaddsbs_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vaddsws_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vaddubm_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vadduwm_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vadduhm_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vaddudm_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vaddubs_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vadduws_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vadduhs_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vaddfp_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsubcuw_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsubshs_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsubsbs_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsubsws_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsububm_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsubuwm_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsubuhm_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsubudm_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsububs_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsubuws_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsubuhs_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsubfp_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmulesb_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmuleub_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmulesh_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmuleuh_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmulosb_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmuloub_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmulosh_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmulosw_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmulouh_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmuluwm_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmhaddshs_PPC(VectorRegister d,VectorRegister a, VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmhraddshs_PPC(VectorRegister d,VectorRegister a,VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmladduhm_PPC(VectorRegister d,VectorRegister a, VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmsubuhm_PPC(VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmsummbm_PPC(VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmsumshm_PPC(VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmsumshs_PPC(VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmsumuhm_PPC(VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmsumuhs_PPC(VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmaddfp_PPC( VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsumsws_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsum2sws_PPC(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsum4sbs_PPC(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsum4ubs_PPC(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsum4shs_PPC(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vavgsb_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vavgsw_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vavgsh_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vavgub_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vavguw_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vavguh_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmaxsb_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmaxsw_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmaxsh_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmaxub_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmaxuw_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmaxuh_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vminsb_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vminsw_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vminsh_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vminub_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vminuw_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vminuh_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpequb_PPC(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpequh_PPC(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpequw_PPC(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtsh_PPC(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtsb_PPC(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtsw_PPC(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtub_PPC(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtuh_PPC(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtuw_PPC(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpequb__PPC(VectorRegister d,VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpequh__PPC(VectorRegister d,VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpequw__PPC(VectorRegister d,VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtsh__PPC(VectorRegister d,VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtsb__PPC(VectorRegister d,VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtsw__PPC(VectorRegister d,VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtub__PPC(VectorRegister d,VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtuh__PPC(VectorRegister d,VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtuw__PPC(VectorRegister d,VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vand_PPC(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vandc_PPC(   VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vnor_PPC(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vor_PPC(     VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmr_PPC(     VectorRegister d, VectorRegister a)                   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vxor_PPC(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vrld_PPC(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vrlb_PPC(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vrlw_PPC(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vrlh_PPC(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vslb_PPC(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vskw_PPC(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vslh_PPC(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsrb_PPC(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsrw_PPC(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsrh_PPC(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsrab_PPC(   VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsraw_PPC(   VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsrah_PPC(   VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpopcntw_PPC(VectorRegister d, VectorRegister b)                   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mtvscr_PPC(  VectorRegister b)                                     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mfvscr_PPC(  VectorRegister d)                                     { illtrap(); } // This is a PPC instruction and should be removed

// AES (introduced with Power 8)
inline void Assembler::vcipher_PPC(     VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcipherlast_PPC( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vncipher_PPC(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vncipherlast_PPC(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsbox_PPC(       VectorRegister d, VectorRegister a)                   { illtrap(); } // This is a PPC instruction and should be removed

// SHA (introduced with Power 8)
inline void Assembler::vshasigmad_PPC(VectorRegister d, VectorRegister a, bool st, int six) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vshasigmaw_PPC(VectorRegister d, VectorRegister a, bool st, int six) { illtrap(); } // This is a PPC instruction and should be removed

// Vector Binary Polynomial Multiplication (introduced with Power 8)
inline void Assembler::vpmsumb_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpmsumd_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpmsumh_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpmsumw_PPC(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed

// Vector Permute and Xor (introduced with Power 8)
inline void Assembler::vpermxor_PPC( VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed

// Transactional Memory instructions (introduced with Power 8)
inline void Assembler::tbegin__PPC()                                { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tbeginrot__PPC()                             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tend__PPC()                                  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tendall__PPC()                               { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tabort__PPC()                                { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tabort__PPC(Register a)                      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tabortwc__PPC(int t, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tabortwci__PPC(int t, Register a, int si)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tabortdc__PPC(int t, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tabortdci__PPC(int t, Register a, int si)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tsuspend__PPC()                              { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tresume__PPC()                               { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tcheck_PPC(int f)                            { illtrap(); } // This is a PPC instruction and should be removed

// Deliver A Random Number (introduced with POWER9)
inline void Assembler::darn_PPC(Register d, int l /* =1 */) { illtrap(); } // This is a PPC instruction and should be removed

// ra0 version
inline void Assembler::lwzx_PPC( Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwz_PPC(  Register d, int si16   ) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwax_PPC( Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwa_PPC(  Register d, int si16   ) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwbrx_PPC(Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lhzx_PPC( Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lhz_PPC(  Register d, int si16   ) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lhax_PPC( Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lha_PPC(  Register d, int si16   ) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lhbrx_PPC(Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lbzx_PPC( Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lbz_PPC(  Register d, int si16   ) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ld_PPC(   Register d, int si16   ) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ldx_PPC(  Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ldbrx_PPC(Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stwx_PPC( Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stw_PPC(  Register d, int si16   ) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stwbrx_PPC(Register d, Register s2){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sthx_PPC( Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sth_PPC(  Register d, int si16   ) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sthbrx_PPC(Register d, Register s2){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stbx_PPC( Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stb_PPC(  Register d, int si16   ) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::std_PPC(  Register d, int si16   ) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stdx_PPC( Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stdbrx_PPC(Register d, Register s2){ illtrap(); } // This is a PPC instruction and should be removed

// ra0 version
inline void Assembler::icbi_PPC(    Register s2)          { illtrap(); } // This is a PPC instruction and should be removed
//inline void Assembler::dcba(  Register s2)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbz_PPC(    Register s2)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbst_PPC(   Register s2)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbf_PPC(    Register s2)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbt_PPC(    Register s2)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbtct_PPC(  Register s2, int ct)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbtds_PPC(  Register s2, int ds)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbtst_PPC(  Register s2)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbtstct_PPC(Register s2, int ct)  { illtrap(); } // This is a PPC instruction and should be removed

// ra0 version
inline void Assembler::lbarx_unchecked_PPC(Register d, Register b, int eh1)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lharx_unchecked_PPC(Register d, Register b, int eh1)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwarx_unchecked_PPC(Register d, Register b, int eh1)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ldarx_unchecked_PPC(Register d, Register b, int eh1)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lqarx_unchecked_PPC(Register d, Register b, int eh1)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lbarx_PPC( Register d, Register b, bool hint_exclusive_access){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lharx_PPC( Register d, Register b, bool hint_exclusive_access){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwarx_PPC( Register d, Register b, bool hint_exclusive_access){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ldarx_PPC( Register d, Register b, bool hint_exclusive_access){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lqarx_PPC( Register d, Register b, bool hint_exclusive_access){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stbcx__PPC(Register s, Register b)                            { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sthcx__PPC(Register s, Register b)                            { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stwcx__PPC(Register s, Register b)                            { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stdcx__PPC(Register s, Register b)                            { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stqcx__PPC(Register s, Register b)                            { illtrap(); } // This is a PPC instruction and should be removed

// ra0 version
inline void Assembler::lfs_PPC( FloatRegister d, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lfsx_PPC(FloatRegister d, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lfd_PPC( FloatRegister d, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lfdx_PPC(FloatRegister d, Register b) { illtrap(); } // This is a PPC instruction and should be removed

// ra0 version
inline void Assembler::stfs_PPC( FloatRegister s, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stfsx_PPC(FloatRegister s, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stfd_PPC( FloatRegister s, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stfdx_PPC(FloatRegister s, Register b) { illtrap(); } // This is a PPC instruction and should be removed

// ra0 version
inline void Assembler::lvebx_PPC( VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvehx_PPC( VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvewx_PPC( VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvx_PPC(   VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvxl_PPC(  VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stvebx_PPC(VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stvehx_PPC(VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stvewx_PPC(VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stvx_PPC(  VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stvxl_PPC( VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvsl_PPC(  VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvsr_PPC(  VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::load_perm_PPC(VectorRegister perm, Register addr) {
#if defined(VM_LITTLE_ENDIAN)
  lvsr_PPC(perm, addr);
#else
  lvsl_PPC(perm, addr);
#endif
}

inline void Assembler::vec_perm_PPC(VectorRegister first_dest, VectorRegister second, VectorRegister perm) {
#if defined(VM_LITTLE_ENDIAN)
  vperm_PPC(first_dest, second, first_dest, perm);
#else
  vperm_PPC(first_dest, first_dest, second, perm);
#endif
}

inline void Assembler::vec_perm_PPC(VectorRegister dest, VectorRegister first, VectorRegister second, VectorRegister perm) {
#if defined(VM_LITTLE_ENDIAN)
  vperm_PPC(dest, second, first, perm);
#else
  vperm_PPC(dest, first, second, perm);
#endif
}

inline void Assembler::load_const_PPC(Register d, void* x, Register tmp) {
   load_const_PPC(d, (long)x, tmp);
}

// Load a 64 bit constant encoded by a `Label'. This works for bound
// labels as well as unbound ones. For unbound labels, the code will
// be patched as soon as the label gets bound.
inline void Assembler::load_const_PPC(Register d, Label& L, Register tmp) {
  load_const_PPC(d, target(L), tmp);
}

// Load a 64 bit constant encoded by an AddressLiteral. patchable.
inline void Assembler::load_const_PPC(Register d, AddressLiteral& a, Register tmp) {
  // First relocate (we don't change the offset in the RelocationHolder,
  // just pass a.rspec()), then delegate to load_const_PPC(Register, long).
  relocate(a.rspec());
  load_const_PPC(d, (long)a.value(), tmp);
}

inline void Assembler::load_const32_PPC(Register d, int i) {
  lis_PPC(d, i >> 16);
  ori_PPC(d, d, i & 0xFFFF);
}

#endif // CPU_RISCV_ASSEMBLER_RISCV_INLINE_HPP
