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

inline void Assembler::op_imm_RV(Register d, Register s, int f, int imm) { emit_int32(OP_IMM_RV_OPCODE | rd(d) | rs1(s) | funct3(f) | immi(imm)); }
inline void Assembler::lui_RV(Register d, int imm) { emit_int32(LUI_RV_OPCODE | rd(d) | immu(imm)); }
inline void Assembler::auipc_RV(Register d, int imm) { emit_int32(AUIPC_RV_OPCODE | rd(d) | immu(imm)); }
inline void Assembler::op_RV(Register d, Register s1, Register s2, int f1, int f2) { emit_int32(OP_RV_OPCODE | rd(d) | rs1(s1) | rs2(s2) | funct3(f1) | funct7(f2)); }
inline void Assembler::amo_RV(Register d, Register s1, Register s2, int f1, int f2, bool aq, bool rl) { emit_int32(AMO_RV_OPCODE | rd(d) | rs1(s1) | rs2(s2) | funct3(f1) | funct7(f2, aq, rl)); }
inline void Assembler::jal_RV(Register d, int off) { emit_int32(JAL_RV_OPCODE | rd(d) | immj(off)); }
inline void Assembler::jalr_RV(Register d, Register base, int off) { emit_int32(JALR_RV_OPCODE | rd(d) | rs1(base) | immi(off)); }
inline void Assembler::branch_RV(Register s1, Register s2, int f, int off) { emit_int32(BRANCH_RV_OPCODE | rs1(s1) | rs2(s2) | funct3(f) | immb(off)); }
inline void Assembler::load_RV(Register d, Register s, int width, int off) { emit_int32(LOAD_RV_OPCODE | rd(d) | rs1(s) | funct3(width) | immi(off)); }
inline void Assembler::load_fp_RV(FloatRegister d, Register s, int width, int off) { emit_int32(LOAD_FP_RV_OPCODE | rd(d) | rs1(s) | funct3(width) | immi(off)); }
inline void Assembler::store_RV(Register base, Register s, int width, int off) { emit_int32(STORE_RV_OPCODE | rs1(base) | rs2(s) | funct3(width) | imms(off)); }
inline void Assembler::store_fp_RV(FloatRegister base, Register s, int width, int off) { emit_int32(STORE_FP_RV_OPCODE | rs1(base) | rs2(s) | funct3(width) | imms(off)); }
inline void Assembler::op_imm32_RV(Register d, Register s, int f, int imm) { emit_int32(OP_IMM32_RV_OPCODE | rd(d) | rs1(s) | funct3(f) | immi(imm)); }
inline void Assembler::op32_RV(Register d, Register s1, Register s2, int f1, int f2) { emit_int32(OP32_RV_OPCODE | rd(d) | rs1(s1) | rs2(s2) | funct3(f1) | funct7(f2)); }
inline void Assembler::op_fp_RV(Register d, Register s1, Register s2, int rm, int f) { emit_int32(OP_FP_RV_OPCODE | rd(d) | rs1(s1) | rs2(s2) | funct3(rm) | funct7(f)); }
inline void Assembler::op_fp_RV(Register d, Register s1, int s2, int rm, int f) { emit_int32(OP_FP_RV_OPCODE | rd(d) | rs1(s1) | rs2(s2) | funct3(rm) | funct7(f)); }
inline void Assembler::madd_RV(Register d, Register s1, Register s2, Register s3, int rm, int f) { emit_int32(MADD_RV_OPCODE | rd(d) | rs1(s1) | rs2(s2) | rs3(s3) | funct3(rm) | funct2(f)); }
inline void Assembler::msub_RV(Register d, Register s1, Register s2, Register s3, int rm, int f) { emit_int32(MSUB_RV_OPCODE | rd(d) | rs1(s1) | rs2(s2) | rs3(s3) | funct3(rm) | funct2(f)); }
inline void Assembler::nmadd_RV(Register d, Register s1, Register s2, Register s3, int rm, int f) { emit_int32(NMSUB_RV_OPCODE | rd(d) | rs1(s1) | rs2(s2) | rs3(s3) | funct3(rm) | funct2(f)); }
inline void Assembler::nmsub_RV(Register d, Register s1, Register s2, Register s3, int rm, int f) { emit_int32(NMADD_RV_OPCODE | rd(d) | rs1(s1) | rs2(s2) | rs3(s3) | funct3(rm) | funct2(f)); }

inline void Assembler::addi_RV(   Register d, Register s, int imm)   { op_imm_RV(d, s, 0x0, imm          ); }
inline void Assembler::slti_RV(   Register d, Register s, int imm)   { op_imm_RV(d, s, 0x2, imm          ); }
inline void Assembler::sltiu_RV(  Register d, Register s, int imm)   { op_imm_RV(d, s, 0x3, imm          ); }
inline void Assembler::xori_RV(   Register d, Register s, int imm)   { op_imm_RV(d, s, 0x4, imm          ); }
inline void Assembler::ori_RV(    Register d, Register s, int imm)   { op_imm_RV(d, s, 0x6, imm          ); }
inline void Assembler::andi_RV(   Register d, Register s, int imm)   { op_imm_RV(d, s, 0x7, imm          ); }
inline void Assembler::slli_RV(   Register d, Register s, int shamt) { op_imm_RV(d, s, 0x1, shamt        ); }
inline void Assembler::srli_RV(   Register d, Register s, int shamt) { op_imm_RV(d, s, 0x5, shamt        ); }
inline void Assembler::srai_RV(   Register d, Register s, int shamt) { op_imm_RV(d, s, 0x5, shamt | 0x400); }

inline void Assembler::add_RV(    Register d, Register s1, Register s2) { op_RV(d, s1, s2, 0x0, 0x0 ); }
inline void Assembler::slt_RV(    Register d, Register s1, Register s2) { op_RV(d, s1, s2, 0x2, 0x0 ); }
inline void Assembler::sltu_RV(   Register d, Register s1, Register s2) { op_RV(d, s1, s2, 0x3, 0x0 ); }
inline void Assembler::andr_RV(   Register d, Register s1, Register s2) { op_RV(d, s1, s2, 0x7, 0x0 ); }
inline void Assembler::orr_RV(    Register d, Register s1, Register s2) { op_RV(d, s1, s2, 0x6, 0x0 ); }
inline void Assembler::xorr_RV(   Register d, Register s1, Register s2) { op_RV(d, s1, s2, 0x4, 0x0 ); }
inline void Assembler::sll_RV(    Register d, Register s1, Register s2) { op_RV(d, s1, s2, 0x1, 0x0 ); }
inline void Assembler::srl_RV(    Register d, Register s1, Register s2) { op_RV(d, s1, s2, 0x5, 0x0 ); }
inline void Assembler::sub_RV(    Register d, Register s1, Register s2) { op_RV(d, s1, s2, 0x0, 0x20); }
inline void Assembler::sra_RV(    Register d, Register s1, Register s2) { op_RV(d, s1, s2, 0x5, 0x20); }
inline void Assembler::mul_RV(    Register d, Register s1, Register s2) { op_RV(d, s1, s2, 0x0, 0x1 ); }
inline void Assembler::mulh_RV(   Register d, Register s1, Register s2) { op_RV(d, s1, s2, 0x1, 0x1 ); }
inline void Assembler::mulhsu_RV( Register d, Register s1, Register s2) { op_RV(d, s1, s2, 0x2, 0x1 ); }
inline void Assembler::mulhu_RV(  Register d, Register s1, Register s2) { op_RV(d, s1, s2, 0x3, 0x1 ); }
inline void Assembler::div_RV(    Register d, Register s1, Register s2) { op_RV(d, s1, s2, 0x4, 0x1 ); }
inline void Assembler::divu_RV(   Register d, Register s1, Register s2) { op_RV(d, s1, s2, 0x5, 0x1 ); }
inline void Assembler::rem_RV(    Register d, Register s1, Register s2) { op_RV(d, s1, s2, 0x6, 0x1 ); }
inline void Assembler::remu_RV(   Register d, Register s1, Register s2) { op_RV(d, s1, s2, 0x7, 0x1 ); }

inline void Assembler::beq_RV(    Register s1, Register s2, int off)  { branch_RV(s1, s2, 0x0, off); }
inline void Assembler::bne_RV(    Register s1, Register s2, int off)  { branch_RV(s1, s2, 0x1, off); }
inline void Assembler::blt_RV(    Register s1, Register s2, int off)  { branch_RV(s1, s2, 0x4, off); }
inline void Assembler::bltu_RV(   Register s1, Register s2, int off)  { branch_RV(s1, s2, 0x6, off); }
inline void Assembler::bge_RV(    Register s1, Register s2, int off)  { branch_RV(s1, s2, 0x5, off); }
inline void Assembler::bgeu_RV(   Register s1, Register s2, int off)  { branch_RV(s1, s2, 0x7, off); }

inline void Assembler::beqz_RV(   Register s, int off)  { beq_RV (s, R0_ZERO_RV, off); }
inline void Assembler::bnez_RV(   Register s, int off)  { bne_RV(s, R0_ZERO_RV, off); }

inline void Assembler::beq_RV(    Register s1, Register s2, Label& L) { beq_RV(s1, s2, disp(intptr_t(target(L)), intptr_t(pc()))); }
inline void Assembler::bne_RV(    Register s1, Register s2, Label& L) { bne_RV(s1, s2, disp(intptr_t(target(L)), intptr_t(pc()))); }

inline void Assembler::beqz_RV(   Register s, Label& L) { beq_RV(s, R0_ZERO_RV, L); }
inline void Assembler::bnez_RV(   Register s, Label& L) { bne_RV(s, R0_ZERO_RV, L); }

inline void Assembler::ld_RV(     Register d, Register s, int off) { load_RV(d, s, 0x3, off); }
inline void Assembler::lw_RV(     Register d, Register s, int off) { load_RV(d, s, 0x2, off); }
inline void Assembler::lwu_RV(    Register d, Register s, int off) { load_RV(d, s, 0x6, off); }
inline void Assembler::lh_RV(     Register d, Register s, int off) { load_RV(d, s, 0x1, off); }
inline void Assembler::lhu_RV(    Register d, Register s, int off) { load_RV(d, s, 0x5, off); }
inline void Assembler::lb_RV(     Register d, Register s, int off) { load_RV(d, s, 0x0, off); }
inline void Assembler::lbu_RV(    Register d, Register s, int off) { load_RV(d, s, 0x4, off); }

inline void Assembler::sd_RV(Register base, Register s, int off) { store_RV(base, s, 0x3, off); }
inline void Assembler::sw_RV(Register base, Register s, int off) { store_RV(base, s, 0x2, off); }
inline void Assembler::sh_RV(Register base, Register s, int off) { store_RV(base, s, 0x1, off); }
inline void Assembler::sb_RV(Register base, Register s, int off) { store_RV(base, s, 0x0, off); }

inline void Assembler::ecall_RV()  { emit_int32(SYSTEM_RV_OPCODE | immi(0x0)); }
inline void Assembler::ebreak_RV() { emit_int32(SYSTEM_RV_OPCODE | immi(0x1)); }

inline void Assembler::addiw_RV(Register d, Register s, int imm  ) { op_imm32_RV(d, s, 0x0, imm          ); }
inline void Assembler::slliw_RV(Register d, Register s, int shamt) { op_imm32_RV(d, s, 0x1, shamt        ); }
inline void Assembler::srliw_RV(Register d, Register s, int shamt) { op_imm32_RV(d, s, 0x5, shamt        ); }
inline void Assembler::sraiw_RV(Register d, Register s, int shamt) { op_imm32_RV(d, s, 0x5, shamt | 0x400); }

inline void Assembler::addw_RV( Register d, Register s1, Register s2) { op32_RV(d, s1, s2, 0x0, 0x0 ); }
inline void Assembler::subw_RV( Register d, Register s1, Register s2) { op32_RV(d, s1, s2, 0x0, 0x20); }
inline void Assembler::sllw_RV( Register d, Register s1, Register s2) { op32_RV(d, s1, s2, 0x1, 0x0 ); }
inline void Assembler::srlw_RV( Register d, Register s1, Register s2) { op32_RV(d, s1, s2, 0x5, 0x0 ); }
inline void Assembler::sraw_RV( Register d, Register s1, Register s2) { op32_RV(d, s1, s2, 0x5, 0x20); }
inline void Assembler::mulw_RV( Register d, Register s1, Register s2) { op32_RV(d, s1, s2, 0x0, 0x1 ); }
inline void Assembler::divw_RV( Register d, Register s1, Register s2) { op32_RV(d, s1, s2, 0x4, 0x1 ); }
inline void Assembler::divuw_RV(Register d, Register s1, Register s2) { op32_RV(d, s1, s2, 0x5, 0x1 ); }
inline void Assembler::remw_RV( Register d, Register s1, Register s2) { op32_RV(d, s1, s2, 0x6, 0x1 ); }
inline void Assembler::remuw_RV(Register d, Register s1, Register s2) { op32_RV(d, s1, s2, 0x7, 0x1 ); }

inline void Assembler::lrw_RV(     Register d, Register s1,              bool aq, bool rl) { amo_RV(d, s1, 0,  0x2, 0x2,  aq, rl); }
inline void Assembler::scw_RV(     Register d, Register s1, Register s2, bool aq, bool rl) { amo_RV(d, s1, s2, 0x2, 0x3,  aq, rl); }
inline void Assembler::amoswapw_RV(Register d, Register s1, Register s2, bool aq, bool rl) { amo_RV(d, s1, s2, 0x2, 0x1,  aq, rl); }
inline void Assembler::amoaddw_RV( Register d, Register s1, Register s2, bool aq, bool rl) { amo_RV(d, s1, s2, 0x2, 0x0,  aq, rl); }
inline void Assembler::amoxorw_RV( Register d, Register s1, Register s2, bool aq, bool rl) { amo_RV(d, s1, s2, 0x2, 0x4,  aq, rl); }
inline void Assembler::amoandw_RV( Register d, Register s1, Register s2, bool aq, bool rl) { amo_RV(d, s1, s2, 0x2, 0xc,  aq, rl); }
inline void Assembler::amoorw_RV(  Register d, Register s1, Register s2, bool aq, bool rl) { amo_RV(d, s1, s2, 0x2, 0x8,  aq, rl); }
inline void Assembler::amominw_RV( Register d, Register s1, Register s2, bool aq, bool rl) { amo_RV(d, s1, s2, 0x2, 0x10, aq, rl); }
inline void Assembler::amomaxw_RV( Register d, Register s1, Register s2, bool aq, bool rl) { amo_RV(d, s1, s2, 0x2, 0x14, aq, rl); }
inline void Assembler::amominuw_RV(Register d, Register s1, Register s2, bool aq, bool rl) { amo_RV(d, s1, s2, 0x2, 0x18, aq, rl); }
inline void Assembler::amomaxuw_RV(Register d, Register s1, Register s2, bool aq, bool rl) { amo_RV(d, s1, s2, 0x2, 0x1c, aq, rl); }

inline void Assembler::lrd_RV(     Register d, Register s1,              bool aq, bool rl) { amo_RV(d, s1, 0,  0x3, 0x2,  aq, rl); }
inline void Assembler::scd_RV(     Register d, Register s1, Register s2, bool aq, bool rl) { amo_RV(d, s1, s2, 0x3, 0x3,  aq, rl); }
inline void Assembler::amoswapd_RV(Register d, Register s1, Register s2, bool aq, bool rl) { amo_RV(d, s1, s2, 0x3, 0x1,  aq, rl); }
inline void Assembler::amoaddd_RV( Register d, Register s1, Register s2, bool aq, bool rl) { amo_RV(d, s1, s2, 0x3, 0x0,  aq, rl); }
inline void Assembler::amoxord_RV( Register d, Register s1, Register s2, bool aq, bool rl) { amo_RV(d, s1, s2, 0x3, 0x4,  aq, rl); }
inline void Assembler::amoandd_RV( Register d, Register s1, Register s2, bool aq, bool rl) { amo_RV(d, s1, s2, 0x3, 0xc,  aq, rl); }
inline void Assembler::amoord_RV(  Register d, Register s1, Register s2, bool aq, bool rl) { amo_RV(d, s1, s2, 0x3, 0x8,  aq, rl); }
inline void Assembler::amomind_RV( Register d, Register s1, Register s2, bool aq, bool rl) { amo_RV(d, s1, s2, 0x3, 0x10, aq, rl); }
inline void Assembler::amomaxd_RV( Register d, Register s1, Register s2, bool aq, bool rl) { amo_RV(d, s1, s2, 0x3, 0x14, aq, rl); }
inline void Assembler::amominud_RV(Register d, Register s1, Register s2, bool aq, bool rl) { amo_RV(d, s1, s2, 0x3, 0x18, aq, rl); }
inline void Assembler::amomaxud_RV(Register d, Register s1, Register s2, bool aq, bool rl) { amo_RV(d, s1, s2, 0x3, 0x1c, aq, rl); }

inline void Assembler::flw_RV(     FloatRegister d,    Register s, int imm) { load_fp_RV( d,    s, 0x2, imm); }
inline void Assembler::fsw_RV(     FloatRegister base, Register s, int imm) { store_fp_RV(base, s, 0x2, imm); }

inline void Assembler::fmadds_RV(  Register d, Register s1, Register s2, Register s3, int rm) { madd_RV(d, s1, s2, s3, rm, 0x0); }
inline void Assembler::fmsubs_RV(  Register d, Register s1, Register s2, Register s3, int rm) { msub_RV(d, s1, s2, s3, rm, 0x0); }
inline void Assembler::fnmadds_RV( Register d, Register s1, Register s2, Register s3, int rm) { nmadd_RV(d, s1, s2, s3, rm, 0x0); }
inline void Assembler::fnmsubs_RV( Register d, Register s1, Register s2, Register s3, int rm) { nmsub_RV(d, s1, s2, s3, rm, 0x0); }
inline void Assembler::fadds_RV(   Register d, Register s1, Register s2, int rm) { op_fp_RV(d, s1, s2, rm, 0x0); }
inline void Assembler::fsubs_RV(   Register d, Register s1, Register s2, int rm) { op_fp_RV(d, s1, s2, rm, 0x4); }
inline void Assembler::fmuls_RV(   Register d, Register s1, Register s2, int rm) { op_fp_RV(d, s1, s2, rm, 0x8); }
inline void Assembler::fdivs_RV(   Register d, Register s1, Register s2, int rm) { op_fp_RV(d, s1, s2, rm, 0xc); }
inline void Assembler::fsqrts_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x0, rm, 0x2c); }
inline void Assembler::fsgnjs_RV(  Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x0, 0x10); }
inline void Assembler::fsgnjns_RV( Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x1, 0x10); }
inline void Assembler::fsgnjxs_RV( Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x2, 0x10); }
inline void Assembler::fmins_RV(   Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x0, 0x14); }
inline void Assembler::fmaxs_RV(   Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x1, 0x14); }
inline void Assembler::fcvtws_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x0, rm, 0x60); }
inline void Assembler::fcvtwus_RV( Register d, Register s, int rm) { op_fp_RV(d, s, 0x1, rm, 0x60); }
inline void Assembler::fmvxw_RV(   Register d, Register s) { op_fp_RV(d, s, 0x0, 0x0, 0x70); }
inline void Assembler::feqs_RV(    Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x2, 0x50); }
inline void Assembler::flts_RV(    Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x1, 0x50); }
inline void Assembler::fles_RV(    Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x0, 0x50); }
inline void Assembler::fclasss_RV( Register d, Register s) { op_fp_RV(d, s, 0x0, 0x1, 0x70); }
inline void Assembler::fcvtsw_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x0, rm, 0x68); }
inline void Assembler::fcvtswu_RV( Register d, Register s, int rm) { op_fp_RV(d, s, 0x1, rm, 0x68); }
inline void Assembler::fmvwx_RV(   Register d, Register s) { op_fp_RV(d, s, 0x0, 0x0, 0x78); }
inline void Assembler::fcvtls_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x2, rm, 0x60); }
inline void Assembler::fcvtlus_RV( Register d, Register s, int rm) { op_fp_RV(d, s, 0x3, rm, 0x60); }
inline void Assembler::fcvtsl_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x2, rm, 0x68); }
inline void Assembler::fcvtslu_RV( Register d, Register s, int rm) { op_fp_RV(d, s, 0x3, rm, 0x68); }

inline void Assembler::fld_RV(     FloatRegister d,    Register s, int imm) { load_fp_RV( d,    s, 0x3, imm); }
inline void Assembler::fsd_RV(     FloatRegister base, Register s, int imm) { store_fp_RV(base, s, 0x3, imm); }

inline void Assembler::fmaddd_RV(  Register d, Register s1, Register s2, Register s3, int rm) { madd_RV(d, s1, s2, s3, rm, 0x1); }
inline void Assembler::fmsubd_RV(  Register d, Register s1, Register s2, Register s3, int rm) { msub_RV(d, s1, s2, s3, rm, 0x1); }
inline void Assembler::fnmaddd_RV( Register d, Register s1, Register s2, Register s3, int rm) { nmadd_RV(d, s1, s2, s3, rm, 0x1); }
inline void Assembler::fnmsubd_RV( Register d, Register s1, Register s2, Register s3, int rm) { nmsub_RV(d, s1, s2, s3, rm, 0x1); }
inline void Assembler::faddd_RV(   Register d, Register s1, Register s2, int rm) { op_fp_RV(d, s1, s2, rm, 0x1); }
inline void Assembler::fsubd_RV(   Register d, Register s1, Register s2, int rm) { op_fp_RV(d, s1, s2, rm, 0x5); }
inline void Assembler::fmuld_RV(   Register d, Register s1, Register s2, int rm) { op_fp_RV(d, s1, s2, rm, 0x9); }
inline void Assembler::fdivd_RV(   Register d, Register s1, Register s2, int rm) { op_fp_RV(d, s1, s2, rm, 0xd); }
inline void Assembler::fsqrtd_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x0, rm, 0x2d); }
inline void Assembler::fsgnjd_RV(  Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x0, 0x11); }
inline void Assembler::fsgnjnd_RV( Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x1, 0x11); }
inline void Assembler::fsgnjxd_RV( Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x2, 0x11); }
inline void Assembler::fmind_RV(   Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x0, 0x15); }
inline void Assembler::fmaxd_RV(   Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x1, 0x15); }
inline void Assembler::fcvtsd_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x1, rm, 0x20); }
inline void Assembler::fcvtds_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x0, rm, 0x21); }
inline void Assembler::feqd_RV(    Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x2, 0x51); }
inline void Assembler::fltd_RV(    Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x1, 0x51); }
inline void Assembler::fled_RV(    Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x0, 0x51); }
inline void Assembler::fclassd_RV( Register d, Register s) { op_fp_RV(d, s, 0x0, 0x1, 0x71); }
inline void Assembler::fcvtwd_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x0, rm, 0x61); }
inline void Assembler::fcvtwud_RV( Register d, Register s, int rm) { op_fp_RV(d, s, 0x1, rm, 0x61); }
inline void Assembler::fcvtdw_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x0, rm, 0x69); }
inline void Assembler::fcvtdwu_RV( Register d, Register s, int rm) { op_fp_RV(d, s, 0x1, rm, 0x69); }
inline void Assembler::fcvtld_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x2, rm, 0x61); }
inline void Assembler::fcvtlud_RV( Register d, Register s, int rm) { op_fp_RV(d, s, 0x3, rm, 0x61); }
inline void Assembler::fcvtdl_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x2, rm, 0x69); }
inline void Assembler::fcvtdlu_RV( Register d, Register s, int rm) { op_fp_RV(d, s, 0x3, rm, 0x69); }
inline void Assembler::fmvxd_RV(   Register d, Register s) { op_fp_RV(d, s, 0x0, 0x0, 0x71); }
inline void Assembler::fmvdx_RV(   Register d, Register s) { op_fp_RV(d, s, 0x0, 0x0, 0x79); }

inline void Assembler::flq_RV(     FloatRegister d,    Register s, int imm) { load_fp_RV( d,    s, 0x4, imm); }
inline void Assembler::fsq_RV(     FloatRegister base, Register s, int imm) { store_fp_RV(base, s, 0x4, imm); }

inline void Assembler::fmaddq_RV(  Register d, Register s1, Register s2, Register s3, int rm) { madd_RV(d, s1, s2, s3, rm, 0x3); }
inline void Assembler::fmsubq_RV(  Register d, Register s1, Register s2, Register s3, int rm) { msub_RV(d, s1, s2, s3, rm, 0x3); }
inline void Assembler::fnmaddq_RV( Register d, Register s1, Register s2, Register s3, int rm) { nmadd_RV(d, s1, s2, s3, rm, 0x3); }
inline void Assembler::fnmsubq_RV( Register d, Register s1, Register s2, Register s3, int rm) { nmsub_RV(d, s1, s2, s3, rm, 0x3); }
inline void Assembler::faddq_RV(   Register d, Register s1, Register s2, int rm) { op_fp_RV(d, s1, s2, rm, 0x3); }
inline void Assembler::fsubq_RV(   Register d, Register s1, Register s2, int rm) { op_fp_RV(d, s1, s2, rm, 0x7); }
inline void Assembler::fmulq_RV(   Register d, Register s1, Register s2, int rm) { op_fp_RV(d, s1, s2, rm, 0xb); }
inline void Assembler::fdivq_RV(   Register d, Register s1, Register s2, int rm) { op_fp_RV(d, s1, s2, rm, 0xf); }
inline void Assembler::fsqrtq_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x0, rm, 0x2f); }
inline void Assembler::fsgnjq_RV(  Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x0, 0x13); }
inline void Assembler::fsgnjnq_RV( Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x1, 0x13); }
inline void Assembler::fsgnjxq_RV( Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x2, 0x13); }
inline void Assembler::fminq_RV(   Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x0, 0x17); }
inline void Assembler::fmaxq_RV(   Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x1, 0x17); }
inline void Assembler::fcvtsq_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x3, rm, 0x20); }
inline void Assembler::fcvtqs_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x0, rm, 0x23); }
inline void Assembler::fcvtdq_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x3, rm, 0x21); }
inline void Assembler::fcvtqd_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x1, rm, 0x23); }
inline void Assembler::feqq_RV(    Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x2, 0x53); }
inline void Assembler::fltq_RV(    Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x1, 0x53); }
inline void Assembler::fleq_RV(    Register d, Register s1, Register s2) { op_fp_RV(d, s1, s2, 0x0, 0x53); }
inline void Assembler::fclassq_RV( Register d, Register s) { op_fp_RV(d, s, 0x0, 0x1, 0x73); }
inline void Assembler::fcvtwq_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x0, rm, 0x63); }
inline void Assembler::fcvtwuq_RV( Register d, Register s, int rm) { op_fp_RV(d, s, 0x1, rm, 0x63); }
inline void Assembler::fcvtqw_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x0, rm, 0x6b); }
inline void Assembler::fcvtqwu_RV( Register d, Register s, int rm) { op_fp_RV(d, s, 0x1, rm, 0x6b); }
inline void Assembler::fcvtlq_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x2, rm, 0x63); }
inline void Assembler::fcvtluq_RV( Register d, Register s, int rm) { op_fp_RV(d, s, 0x3, rm, 0x63); }
inline void Assembler::fcvtql_RV(  Register d, Register s, int rm) { op_fp_RV(d, s, 0x2, rm, 0x6b); }
inline void Assembler::fcvtqlu_RV( Register d, Register s, int rm) { op_fp_RV(d, s, 0x3, rm, 0x6b); }
// pseudoinstructions
inline void Assembler::nop_RV() { addi_RV(R0, R0, 0); }
inline void Assembler::j_RV(int off) { jal_RV(R0, off); }
inline void Assembler::jal_RV(int off) { jal_RV(R1, off); }
inline void Assembler::jr_RV(Register s) { jalr_RV(R0, s, 0); }
inline void Assembler::jalr_RV(Register s) { jalr_RV(R1, s, 0); }
inline void Assembler::ret_RV() { jr_RV(R1); }
inline void Assembler::call_RV(int off) { auipc_RV(R1, off & 0xfffff000); jalr_RV(R1, R1, off & 0xfff); }
inline void Assembler::tail_RV(int off) { auipc_RV(R6, off & 0xfffff000); jalr_RV(R0, R6, off & 0xfff); }
inline void Assembler::neg_RV(Register d, Register s) { sub_RV(d, R0, s); }
inline void Assembler::mv_RV(Register d, Register s) { addi_RV(d, s, 0); }

// --- PPC instructions follow ---

// Issue an illegal instruction. 0 is guaranteed to be an illegal instruction.
inline void Assembler::illtrap() { Assembler::emit_int32(0); } // FIXME_RISCV set more weird number
inline bool Assembler::is_illtrap(int x) { return x == 0; }

// RISCV 1, section 3.3.8, Fixed-Point Arithmetic Instructions
inline void Assembler::addi(   Register d, Register a, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addis(  Register d, Register a, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addi_r0ok(Register d,Register a,int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addis_r0ok(Register d,Register a,int si16)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addic_( Register d, Register a, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfic( Register d, Register a, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::add(    Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::add_(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subf(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sub(    Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subf_(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addc(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addc_(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfc(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfc_( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::adde(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::adde_(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfe(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfe_( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addme(  Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addme_( Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfme( Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfme_(Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addze(  Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addze_( Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfze( Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfze_(Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::neg(    Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::neg_(   Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulli(  Register d, Register a, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulld(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulld_( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mullw(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mullw_( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulhw(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulhw_( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulhwu( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulhwu_(Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulhd(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulhd_( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulhdu( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulhdu_(Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::divd(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::divd_(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::divw(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::divw_(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed

// Fixed-Point Arithmetic Instructions with Overflow detection
inline void Assembler::addo(    Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addo_(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfo(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfo_(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addco(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addco_(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfco(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfco_( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addeo(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addeo_(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfeo(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfeo_( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addmeo(  Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addmeo_( Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfmeo( Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfmeo_(Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addzeo(  Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addzeo_( Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfzeo( Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subfzeo_(Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::nego(    Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::nego_(   Register d, Register a)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulldo(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mulldo_( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mullwo(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mullwo_( Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::divdo(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::divdo_(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::divwo(   Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::divwo_(  Register d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed

// extended mnemonics
inline void Assembler::li(   Register d, int si16)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lis(  Register d, int si16)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::addir(Register d, int si16, Register a) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::subi( Register d, Register a, int si16) { illtrap(); } // This is a PPC instruction and should be removed

// RISCV 1, section 3.3.9, Fixed-Point Compare Instructions
inline void Assembler::cmpi(  ConditionRegister f, int l, Register a, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmp(   ConditionRegister f, int l, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmpli( ConditionRegister f, int l, Register a, int ui16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmpl(  ConditionRegister f, int l, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmprb( ConditionRegister f, int l, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmpeqb(ConditionRegister f, Register a, Register b)        { illtrap(); } // This is a PPC instruction and should be removed

// extended mnemonics of Compare Instructions
inline void Assembler::cmpwi( ConditionRegister crx, Register a, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmpdi( ConditionRegister crx, Register a, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmpw(  ConditionRegister crx, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmpd(  ConditionRegister crx, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmplwi(ConditionRegister crx, Register a, int ui16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmpldi(ConditionRegister crx, Register a, int ui16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmplw( ConditionRegister crx, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cmpld( ConditionRegister crx, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::isel(Register d, Register a, Register b, int c) { illtrap(); } // This is a PPC instruction and should be removed

// RISCV 1, section 3.3.11, Fixed-Point Logical Instructions
inline void Assembler::andi_(   Register a, Register s, int ui16)      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::andis_(  Register a, Register s, int ui16)      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ori(     Register a, Register s, int ui16)      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::oris(    Register a, Register s, int ui16)      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xori(    Register a, Register s, int ui16)      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xoris(   Register a, Register s, int ui16)      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::andr(    Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::and_(    Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::or_unchecked(Register a, Register s, Register b){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::orr(     Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::or_(     Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xorr(    Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xor_(    Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::nand(    Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::nand_(   Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::nor(     Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::nor_(    Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::andc(    Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::andc_(   Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::orc(     Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::orc_(    Register a, Register s, Register b)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::extsb(   Register a, Register s)                { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::extsb_(  Register a, Register s)                { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::extsh(   Register a, Register s)                { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::extsh_(  Register a, Register s)                { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::extsw(   Register a, Register s)                { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::extsw_(  Register a, Register s)                { illtrap(); } // This is a PPC instruction and should be removed

// extended mnemonics
inline void Assembler::nop()                              { illtrap(); } // This is a PPC instruction and should be removed
// NOP for FP and BR units (different versions to allow them to be in one group)
inline void Assembler::fpnop0()                           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fpnop1()                           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::brnop0()                           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::brnop1()                           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::brnop2()                           { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::mr(      Register d, Register s)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ori_opt( Register d, int ui16)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::oris_opt(Register d, int ui16)     { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::endgroup()                         { illtrap(); } // This is a PPC instruction and should be removed

// count instructions
inline void Assembler::cntlzw(  Register a, Register s)              { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cntlzw_( Register a, Register s)              { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cntlzd(  Register a, Register s)              { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cntlzd_( Register a, Register s)              { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cnttzw(  Register a, Register s)              { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cnttzw_( Register a, Register s)              { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cnttzd(  Register a, Register s)              { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cnttzd_( Register a, Register s)              { illtrap(); } // This is a PPC instruction and should be removed

// RISCV 1, section 3.3.12, Fixed-Point Rotate and Shift Instructions
inline void Assembler::sld(     Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sld_(    Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::slw(     Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::slw_(    Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srd(     Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srd_(    Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srw(     Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srw_(    Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srad(    Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srad_(   Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sraw(    Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sraw_(   Register a, Register s, Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sradi(   Register a, Register s, int sh6)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sradi_(  Register a, Register s, int sh6)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srawi(   Register a, Register s, int sh5)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srawi_(  Register a, Register s, int sh5)     { illtrap(); } // This is a PPC instruction and should be removed

// extended mnemonics for Shift Instructions
inline void Assembler::sldi(    Register a, Register s, int sh6)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sldi_(   Register a, Register s, int sh6)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::slwi(    Register a, Register s, int sh5)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::slwi_(   Register a, Register s, int sh5)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srdi(    Register a, Register s, int sh6)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srdi_(   Register a, Register s, int sh6)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srwi(    Register a, Register s, int sh5)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::srwi_(   Register a, Register s, int sh5)     { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::clrrdi(  Register a, Register s, int ui6)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::clrrdi_( Register a, Register s, int ui6)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::clrldi(  Register a, Register s, int ui6)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::clrldi_( Register a, Register s, int ui6)     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::clrlsldi( Register a, Register s, int clrl6, int shl6) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::clrlsldi_(Register a, Register s, int clrl6, int shl6) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::extrdi(  Register a, Register s, int n, int b){ illtrap(); } // This is a PPC instruction and should be removed
// testbit with condition register.
inline void Assembler::testbitdi(ConditionRegister cr, Register a, Register s, int ui6) { illtrap(); } // This is a PPC instruction and should be removed

// rotate instructions
inline void Assembler::rotldi( Register a, Register s, int n) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rotrdi( Register a, Register s, int n) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rotlwi( Register a, Register s, int n) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rotrwi( Register a, Register s, int n) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::rldic(   Register a, Register s, int sh6, int mb6)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rldic_(  Register a, Register s, int sh6, int mb6)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rldicr(  Register a, Register s, int sh6, int mb6)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rldicr_( Register a, Register s, int sh6, int mb6)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rldicl(  Register a, Register s, int sh6, int me6)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rldicl_( Register a, Register s, int sh6, int me6)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rlwinm(  Register a, Register s, int sh5, int mb5, int me5){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rlwinm_( Register a, Register s, int sh5, int mb5, int me5){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rldimi(  Register a, Register s, int sh6, int mb6)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rlwimi(  Register a, Register s, int sh5, int mb5, int me5){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::rldimi_( Register a, Register s, int sh6, int mb6)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::insrdi(  Register a, Register s, int n,   int b)           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::insrwi(  Register a, Register s, int n,   int b)           { illtrap(); } // This is a PPC instruction and should be removed

// RISCV 1, section 3.3.2 Fixed-Point Load Instructions
inline void Assembler::lwzx( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwz(  Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwzu( Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::lwax( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwa(  Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::lwbrx( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::lhzx( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lhz(  Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lhzu( Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::lhbrx( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::lhax( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lha(  Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lhau( Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::lbzx( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lbz(  Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lbzu( Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::ld(   Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ldx(  Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ldu(  Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ldbrx( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::ld_ptr(Register d, int b, Register s1) { illtrap(); } // This is a PPC instruction and should be removed
DEBUG_ONLY(inline void Assembler::ld_ptr(Register d, ByteSize b, Register s1) { illtrap(); }) // This is a PPC instruction and should be removed

//  RISCV 1, section 3.3.3 Fixed-Point Store Instructions
inline void Assembler::stwx( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stw(  Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stwu( Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stwbrx( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::sthx( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sth(  Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sthu( Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sthbrx( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::stbx( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stb(  Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stbu( Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::std(  Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stdx( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stdu( Register d, int si16,    Register s1) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stdux(Register s, Register a,  Register b)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stdbrx( Register d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::st_ptr(Register d, int b, Register s1) { illtrap(); } // This is a PPC instruction and should be removed
DEBUG_ONLY(inline void Assembler::st_ptr(Register d, ByteSize b, Register s1) { illtrap(); }) // This is a PPC instruction and should be removed

// RISCV 1, section 3.3.13 Move To/From System Register Instructions
inline void Assembler::mtlr( Register s1)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mflr( Register d )         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mtctr(Register s1)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mfctr(Register d )         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mtcrf(int afxm, Register s){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mfcr( Register d )         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mcrf( ConditionRegister crd, ConditionRegister cra)
                                                      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mtcr( Register s)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::setb(Register d, ConditionRegister cra)
                                                  { illtrap(); } // This is a PPC instruction and should be removed

// Special purpose registers
// Exception Register
inline void Assembler::mtxer(Register s1)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mfxer(Register d )         { illtrap(); } // This is a PPC instruction and should be removed
// Vector Register Save Register
inline void Assembler::mtvrsave(Register s1)      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mfvrsave(Register d )      { illtrap(); } // This is a PPC instruction and should be removed
// Timebase
inline void Assembler::mftb(Register d )          { illtrap(); } // This is a PPC instruction and should be removed
// Introduced with Power 8:
// Data Stream Control Register
inline void Assembler::mtdscr(Register s1)        { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mfdscr(Register d )        { illtrap(); } // This is a PPC instruction and should be removed
// Transactional Memory Registers
inline void Assembler::mftfhar(Register d )       { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mftfiar(Register d )       { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mftexasr(Register d )      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mftexasru(Register d )     { illtrap(); } // This is a PPC instruction and should be removed

// SAP JVM 2006-02-13 RISCV branch instruction.
// RISCV 1, section 2.4.1 Branch Instructions
inline void Assembler::b( address a, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::b( Label& L)                           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bl(address a, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bl(Label& L)                           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bc( int boint, int biint, address a, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bc( int boint, int biint, Label& L)                           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bcl(int boint, int biint, address a, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bcl(int boint, int biint, Label& L)                           { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::bclr(  int boint, int biint, int bhint, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bclrl( int boint, int biint, int bhint, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bcctr( int boint, int biint, int bhint, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bcctrl(int boint, int biint, int bhint, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed

// helper function for b
inline bool Assembler::is_within_range_of_b(address a, address pc) {
  // Guard against illegal branch targets, e.g. -1 (see CompiledStaticCall and ad-file).
  if ((((uint64_t)a) & 0x3) != 0) return false;

  const int range = 1 << (29-6); // li field is from bit 6 to bit 29.
  int value = disp(intptr_t(a), intptr_t(pc));
  bool result = -range <= value && value < range-1;
#ifdef ASSERT
  if (result) li(value); // Assert that value is in correct range.
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
address  Assembler::bxx_destination(address baddr) { return bxx_destination(*(int*)baddr, baddr); }
address  Assembler::bxx_destination(int instr, address pc) { return (address)bxx_destination_offset(instr, (intptr_t)pc); }
intptr_t Assembler::bxx_destination_offset(int instr, intptr_t bxx_pos) {
  intptr_t displ = inv_li_field(instr);
  return bxx_pos + displ;
}

// Extended mnemonics for Branch Instructions
inline void Assembler::blt(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bgt(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::beq(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bso(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bge(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ble(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bne(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bns(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed

// Branch instructions with static prediction hints.
inline void Assembler::blt_predict_taken    (ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bgt_predict_taken    (ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::beq_predict_taken    (ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bso_predict_taken    (ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bge_predict_taken    (ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ble_predict_taken    (ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bne_predict_taken    (ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bns_predict_taken    (ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::blt_predict_not_taken(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bgt_predict_not_taken(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::beq_predict_not_taken(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bso_predict_not_taken(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bge_predict_not_taken(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ble_predict_not_taken(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bne_predict_not_taken(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bns_predict_not_taken(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed

// For use in conjunction with testbitdi:
inline void Assembler::btrue( ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bfalse(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::bltl(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bgtl(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::beql(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bsol(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bgel(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::blel(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bnel(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bnsl(ConditionRegister crx, Label& L) { illtrap(); } // This is a PPC instruction and should be removed

// Extended mnemonics for Branch Instructions via LR.
// We use `blr' for returns.
inline void Assembler::blr(relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed

// Extended mnemonics for Branch Instructions with CTR.
// Bdnz means `decrement CTR and jump to L if CTR is not zero'.
inline void Assembler::bdnz(Label& L) { illtrap(); } // This is a PPC instruction and should be removed
// Decrement and branch if result is zero.
inline void Assembler::bdz(Label& L)  { illtrap(); } // This is a PPC instruction and should be removed
// We use `bctr[l]' for jumps/calls in function descriptor glue
// code, e.g. for calls to runtime functions.
inline void Assembler::bctr( relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bctrl(relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
// Conditional jumps/branches via CTR.
inline void Assembler::beqctr( ConditionRegister crx, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::beqctrl(ConditionRegister crx, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bnectr( ConditionRegister crx, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::bnectrl(ConditionRegister crx, relocInfo::relocType rt) { illtrap(); } // This is a PPC instruction and should be removed

// condition register logic instructions
inline void Assembler::crand( int d, int s1, int s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::crnand(int d, int s1, int s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::cror(  int d, int s1, int s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::crxor( int d, int s1, int s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::crnor( int d, int s1, int s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::creqv( int d, int s1, int s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::crandc(int d, int s1, int s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::crorc( int d, int s1, int s2) { illtrap(); } // This is a PPC instruction and should be removed

// More convenient version.
inline void Assembler::crand( ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc) {
  int dst_bit = condition_register_bit(crdst, cdst),
      src_bit = condition_register_bit(crsrc, csrc);
  crand(dst_bit, src_bit, dst_bit);
}
inline void Assembler::crnand(ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc) {
  int dst_bit = condition_register_bit(crdst, cdst),
      src_bit = condition_register_bit(crsrc, csrc);
  crnand(dst_bit, src_bit, dst_bit);
}
inline void Assembler::cror(  ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc) {
  int dst_bit = condition_register_bit(crdst, cdst),
      src_bit = condition_register_bit(crsrc, csrc);
  cror(dst_bit, src_bit, dst_bit);
}
inline void Assembler::crxor( ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc) {
  int dst_bit = condition_register_bit(crdst, cdst),
      src_bit = condition_register_bit(crsrc, csrc);
  crxor(dst_bit, src_bit, dst_bit);
}
inline void Assembler::crnor( ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc) {
  int dst_bit = condition_register_bit(crdst, cdst),
      src_bit = condition_register_bit(crsrc, csrc);
  crnor(dst_bit, src_bit, dst_bit);
}
inline void Assembler::creqv( ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc) {
  int dst_bit = condition_register_bit(crdst, cdst),
      src_bit = condition_register_bit(crsrc, csrc);
  creqv(dst_bit, src_bit, dst_bit);
}
inline void Assembler::crandc(ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc) {
  int dst_bit = condition_register_bit(crdst, cdst),
      src_bit = condition_register_bit(crsrc, csrc);
  crandc(dst_bit, src_bit, dst_bit);
}
inline void Assembler::crorc( ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc) {
  int dst_bit = condition_register_bit(crdst, cdst),
      src_bit = condition_register_bit(crsrc, csrc);
  crorc(dst_bit, src_bit, dst_bit);
}

// Conditional move (>= Power7)
inline void Assembler::isel(Register d, ConditionRegister cr, Condition cc, bool inv, Register a, Register b) {
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
  isel(d, first, second, bi0(cr, cc));
}
inline void Assembler::isel_0(Register d, ConditionRegister cr, Condition cc, Register b) {
  if (b == noreg) {
    b = d; // Can be omitted if old value should be kept in "else" case.
  }
  isel(d, R0, b, bi0(cr, cc));
}

// RISCV 2, section 3.2.1 Instruction Cache Instructions
inline void Assembler::icbi(    Register s1, Register s2)         { illtrap(); } // This is a PPC instruction and should be removed
// RISCV 2, section 3.2.2 Data Cache Instructions
//inline void Assembler::dcba(  Register s1, Register s2)         { emit_int32( DCBA_OPCODE   | ra0mem(s1) | rb(s2)           ); }
inline void Assembler::dcbz(    Register s1, Register s2)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbst(   Register s1, Register s2)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbf(    Register s1, Register s2)         { illtrap(); } // This is a PPC instruction and should be removed
// dcache read hint
inline void Assembler::dcbt(    Register s1, Register s2)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbtct(  Register s1, Register s2, int ct) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbtds(  Register s1, Register s2, int ds) { illtrap(); } // This is a PPC instruction and should be removed
// dcache write hint
inline void Assembler::dcbtst(  Register s1, Register s2)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbtstct(Register s1, Register s2, int ct) { illtrap(); } // This is a PPC instruction and should be removed

// machine barrier instructions:
inline void Assembler::sync(int a) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sync()      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwsync()    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ptesync()   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::eieio()     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::isync()     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::elemental_membar(int e) { illtrap(); } // This is a PPC instruction and should be removed

// Wait instructions for polling.
inline void Assembler::wait()    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::waitrsv() { illtrap(); } // This is a PPC instruction and should be removed // WC=0b01 >=Power7

// atomics
// Use ra0mem to disallow R0 as base.
inline void Assembler::lbarx_unchecked(Register d, Register a, Register b, int eh1)           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lharx_unchecked(Register d, Register a, Register b, int eh1)           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwarx_unchecked(Register d, Register a, Register b, int eh1)           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ldarx_unchecked(Register d, Register a, Register b, int eh1)           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lqarx_unchecked(Register d, Register a, Register b, int eh1)           { illtrap(); } // This is a PPC instruction and should be removed
inline bool Assembler::lxarx_hint_exclusive_access()                                          { return VM_Version::has_lxarxeh(); }
inline void Assembler::lbarx( Register d, Register a, Register b, bool hint_exclusive_access) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lharx( Register d, Register a, Register b, bool hint_exclusive_access) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwarx( Register d, Register a, Register b, bool hint_exclusive_access) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ldarx( Register d, Register a, Register b, bool hint_exclusive_access) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lqarx( Register d, Register a, Register b, bool hint_exclusive_access) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stbcx_(Register s, Register a, Register b)                             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sthcx_(Register s, Register a, Register b)                             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stwcx_(Register s, Register a, Register b)                             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stdcx_(Register s, Register a, Register b)                             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stqcx_(Register s, Register a, Register b)                             { illtrap(); } // This is a PPC instruction and should be removed

// Instructions for adjusting thread priority
// for simultaneous multithreading (SMT) on >= POWER5.
inline void Assembler::smt_prio_very_low()    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::smt_prio_low()         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::smt_prio_medium_low()  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::smt_prio_medium()      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::smt_prio_medium_high() { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::smt_prio_high()        { illtrap(); } // This is a PPC instruction and should be removed
// >= Power7
inline void Assembler::smt_yield()            { illtrap(); } // This is a PPC instruction and should be removed // never actually implemented
inline void Assembler::smt_mdoio()            { illtrap(); } // This is a PPC instruction and should be removed // never actually implemetned
inline void Assembler::smt_mdoom()            { illtrap(); } // This is a PPC instruction and should be removed // never actually implemented
// Power8
inline void Assembler::smt_miso()             { illtrap(); } // This is a PPC instruction and should be removed // never actually implemented

inline void Assembler::twi_0(Register a)      { illtrap(); } // This is a PPC instruction and should be removed

// trap instructions
inline void Assembler::tdi_unchecked(int tobits, Register a, int si16){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::twi_unchecked(int tobits, Register a, int si16){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tdi(int tobits, Register a, int si16)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::twi(int tobits, Register a, int si16)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::td( int tobits, Register a, Register b)        { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tw( int tobits, Register a, Register b)        { illtrap(); } // This is a PPC instruction and should be removed

// FLOATING POINT instructions riscv.
// RISCV 1, section 4.6.2 Floating-Point Load Instructions
// Use ra0mem instead of ra in some instructions below.
inline void Assembler::lfs( FloatRegister d, int si16, Register a)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lfsu(FloatRegister d, int si16, Register a)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lfsx(FloatRegister d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lfd( FloatRegister d, int si16, Register a)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lfdu(FloatRegister d, int si16, Register a)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lfdx(FloatRegister d, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed

// RISCV 1, section 4.6.3 Floating-Point Store Instructions
// Use ra0mem instead of ra in some instructions below.
inline void Assembler::stfs( FloatRegister s, int si16, Register a)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stfsu(FloatRegister s, int si16, Register a)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stfsx(FloatRegister s, Register a, Register b){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stfd( FloatRegister s, int si16, Register a)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stfdu(FloatRegister s, int si16, Register a)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stfdx(FloatRegister s, Register a, Register b){ illtrap(); } // This is a PPC instruction and should be removed

// RISCV 1, section 4.6.4 Floating-Point Move Instructions
inline void Assembler::fmr( FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmr_(FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed

// These are special Power6 opcodes, reused for "lfdepx" and "stfdepx"
// on Power7.  Do not use.
//inline void Assembler::mffgpr( FloatRegister d, Register b)   { emit_int32( MFFGPR_OPCODE | frt(d) | rb(b) | rc(0)); }
//inline void Assembler::mftgpr( Register d, FloatRegister b)   { emit_int32( MFTGPR_OPCODE | rt(d) | frb(b) | rc(0)); }
// add cmpb and popcntb to detect riscv power version.
inline void Assembler::cmpb(   Register a, Register s, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::popcntb(Register a, Register s)             { illtrap(); } // This is a PPC instruction and should be removed;
inline void Assembler::popcntw(Register a, Register s)             { illtrap(); } // This is a PPC instruction and should be removed;
inline void Assembler::popcntd(Register a, Register s)             { illtrap(); } // This is a PPC instruction and should be removed;

inline void Assembler::fneg(  FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fneg_( FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fabs(  FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fabs_( FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fnabs( FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fnabs_(FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed

// RISCV 1, section 4.6.5.1 Floating-Point Elementary Arithmetic Instructions
inline void Assembler::fadd(  FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fadd_( FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fadds( FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fadds_(FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fsub(  FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fsub_( FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fsubs( FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fsubs_(FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmul(  FloatRegister d, FloatRegister a, FloatRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmul_( FloatRegister d, FloatRegister a, FloatRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmuls( FloatRegister d, FloatRegister a, FloatRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmuls_(FloatRegister d, FloatRegister a, FloatRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fdiv(  FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fdiv_( FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fdivs( FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fdivs_(FloatRegister d, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed

// Fused multiply-accumulate instructions.
// WARNING: Use only when rounding between the 2 parts is not desired.
// Some floating point tck tests will fail if used incorrectly.
inline void Assembler::fmadd(   FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmadd_(  FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmadds(  FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmadds_( FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmsub(   FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmsub_(  FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmsubs(  FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fmsubs_( FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fnmadd(  FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fnmadd_( FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fnmadds( FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fnmadds_(FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fnmsub(  FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fnmsub_( FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fnmsubs( FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fnmsubs_(FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed

// RISCV 1, section 4.6.6 Floating-Point Rounding and Conversion Instructions
inline void Assembler::frsp(  FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fctid( FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fctidz(FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fctiw( FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fctiwz(FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fcfid( FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fcfids(FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed

// RISCV 1, section 4.6.7 Floating-Point Compare Instructions
inline void Assembler::fcmpu( ConditionRegister crx, FloatRegister a, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed

// RISCV 1, section 5.2.1 Floating-Point Arithmetic Instructions
inline void Assembler::fsqrt( FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::fsqrts(FloatRegister d, FloatRegister b) { illtrap(); } // This is a PPC instruction and should be removed

// Vector instructions for >= Power6.
inline void Assembler::lvebx( VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvehx( VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvewx( VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvx(   VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvxl(  VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stvebx(VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stvehx(VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stvewx(VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stvx(  VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stvxl( VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvsl(  VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvsr(  VectorRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed

// Vector-Scalar (VSX) instructions.
inline void Assembler::lxvd2x(  VectorSRegister d, Register s1)              { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lxvd2x(  VectorSRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stxvd2x( VectorSRegister d, Register s1)              { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stxvd2x( VectorSRegister d, Register s1, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mtvsrd(  VectorSRegister d, Register a)               { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mtvsrwz( VectorSRegister d, Register a)               { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xxspltw( VectorSRegister d, VectorSRegister b, int ui2)           { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xxlor(   VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xxlxor(  VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xxleqv(  VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvdivsp( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvdivdp( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvabssp( VectorSRegister d, VectorSRegister b)                    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvabsdp( VectorSRegister d, VectorSRegister b)                    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvnegsp( VectorSRegister d, VectorSRegister b)                    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvnegdp( VectorSRegister d, VectorSRegister b)                    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvsqrtsp(VectorSRegister d, VectorSRegister b)                    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvsqrtdp(VectorSRegister d, VectorSRegister b)                    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xscvdpspn(VectorSRegister d, VectorSRegister b)                   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvadddp( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvsubdp( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvmulsp( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvmuldp( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvmaddasp( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvmaddadp( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvmsubasp( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvmsubadp( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvnmsubasp(VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xvnmsubadp(VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mtvrd(   VectorRegister d, Register a)               { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mfvrd(   Register        a, VectorRegister d)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mtvrwz(  VectorRegister  d, Register a)               { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mfvrwz(  Register        a, VectorRegister d)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xxpermdi(VectorSRegister d, VectorSRegister a, VectorSRegister b, int dm) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xxmrghw( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xxmrglw( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed

// VSX Extended Mnemonics
inline void Assembler::xxspltd( VectorSRegister d, VectorSRegister a, int x)             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xxmrghd( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xxmrgld( VectorSRegister d, VectorSRegister a, VectorSRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::xxswapd( VectorSRegister d, VectorSRegister a)                    { illtrap(); } // This is a PPC instruction and should be removed

// Vector-Scalar (VSX) instructions.
inline void Assembler::mtfprd(  FloatRegister   d, Register a)      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mtfprwa( FloatRegister   d, Register a)      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mffprd(  Register        a, FloatRegister d) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::vpkpx(   VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpkshss( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpkswss( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpkshus( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpkswus( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpkuhum( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpkuwum( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpkuhus( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpkuwus( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vupkhpx( VectorRegister d, VectorRegister b)                   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vupkhsb( VectorRegister d, VectorRegister b)                   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vupkhsh( VectorRegister d, VectorRegister b)                   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vupklpx( VectorRegister d, VectorRegister b)                   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vupklsb( VectorRegister d, VectorRegister b)                   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vupklsh( VectorRegister d, VectorRegister b)                   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmrghb(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmrghw(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmrghh(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmrglb(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmrglw(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmrglh(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsplt(   VectorRegister d, int ui4,          VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsplth(  VectorRegister d, int ui3,          VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vspltw(  VectorRegister d, int ui2,          VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vspltisb(VectorRegister d, int si5)                            { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vspltish(VectorRegister d, int si5)                            { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vspltisw(VectorRegister d, int si5)                            { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vperm(   VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsel(    VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsl(     VectorRegister d, VectorRegister a, VectorRegister b)                  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsldoi(  VectorRegister d, VectorRegister a, VectorRegister b, int ui4)         { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vslo(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsr(     VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsro(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vaddcuw( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vaddshs( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vaddsbs( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vaddsws( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vaddubm( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vadduwm( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vadduhm( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vaddudm( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vaddubs( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vadduws( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vadduhs( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vaddfp(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsubcuw( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsubshs( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsubsbs( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsubsws( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsububm( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsubuwm( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsubuhm( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsubudm( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsububs( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsubuws( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsubuhs( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsubfp(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmulesb( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmuleub( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmulesh( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmuleuh( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmulosb( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmuloub( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmulosh( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmulosw( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmulouh( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmuluwm( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmhaddshs(VectorRegister d,VectorRegister a, VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmhraddshs(VectorRegister d,VectorRegister a,VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmladduhm(VectorRegister d,VectorRegister a, VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmsubuhm(VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmsummbm(VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmsumshm(VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmsumshs(VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmsumuhm(VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmsumuhs(VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmaddfp( VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsumsws( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsum2sws(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsum4sbs(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsum4ubs(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsum4shs(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vavgsb(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vavgsw(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vavgsh(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vavgub(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vavguw(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vavguh(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmaxsb(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmaxsw(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmaxsh(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmaxub(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmaxuw(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmaxuh(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vminsb(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vminsw(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vminsh(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vminub(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vminuw(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vminuh(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpequb(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpequh(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpequw(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtsh(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtsb(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtsw(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtub(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtuh(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtuw(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpequb_(VectorRegister d,VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpequh_(VectorRegister d,VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpequw_(VectorRegister d,VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtsh_(VectorRegister d,VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtsb_(VectorRegister d,VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtsw_(VectorRegister d,VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtub_(VectorRegister d,VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtuh_(VectorRegister d,VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcmpgtuw_(VectorRegister d,VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vand(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vandc(   VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vnor(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vor(     VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vmr(     VectorRegister d, VectorRegister a)                   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vxor(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vrld(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vrlb(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vrlw(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vrlh(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vslb(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vskw(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vslh(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsrb(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsrw(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsrh(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsrab(   VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsraw(   VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsrah(   VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpopcntw(VectorRegister d, VectorRegister b)                   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mtvscr(  VectorRegister b)                                     { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::mfvscr(  VectorRegister d)                                     { illtrap(); } // This is a PPC instruction and should be removed

// AES (introduced with Power 8)
inline void Assembler::vcipher(     VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vcipherlast( VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vncipher(    VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vncipherlast(VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vsbox(       VectorRegister d, VectorRegister a)                   { illtrap(); } // This is a PPC instruction and should be removed

// SHA (introduced with Power 8)
inline void Assembler::vshasigmad(VectorRegister d, VectorRegister a, bool st, int six) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vshasigmaw(VectorRegister d, VectorRegister a, bool st, int six) { illtrap(); } // This is a PPC instruction and should be removed

// Vector Binary Polynomial Multiplication (introduced with Power 8)
inline void Assembler::vpmsumb(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpmsumd(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpmsumh(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::vpmsumw(  VectorRegister d, VectorRegister a, VectorRegister b) { illtrap(); } // This is a PPC instruction and should be removed

// Vector Permute and Xor (introduced with Power 8)
inline void Assembler::vpermxor( VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c) { illtrap(); } // This is a PPC instruction and should be removed

// Transactional Memory instructions (introduced with Power 8)
inline void Assembler::tbegin_()                                { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tbeginrot_()                             { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tend_()                                  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tendall_()                               { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tabort_()                                { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tabort_(Register a)                      { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tabortwc_(int t, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tabortwci_(int t, Register a, int si)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tabortdc_(int t, Register a, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tabortdci_(int t, Register a, int si)    { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tsuspend_()                              { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tresume_()                               { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::tcheck(int f)                            { illtrap(); } // This is a PPC instruction and should be removed

// Deliver A Random Number (introduced with POWER9)
inline void Assembler::darn(Register d, int l /* =1 */) { illtrap(); } // This is a PPC instruction and should be removed

// ra0 version
inline void Assembler::lwzx( Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwz(  Register d, int si16   ) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwax( Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwa(  Register d, int si16   ) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwbrx(Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lhzx( Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lhz(  Register d, int si16   ) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lhax( Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lha(  Register d, int si16   ) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lhbrx(Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lbzx( Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lbz(  Register d, int si16   ) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ld(   Register d, int si16   ) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ldx(  Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ldbrx(Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stwx( Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stw(  Register d, int si16   ) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stwbrx(Register d, Register s2){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sthx( Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sth(  Register d, int si16   ) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sthbrx(Register d, Register s2){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stbx( Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stb(  Register d, int si16   ) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::std(  Register d, int si16   ) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stdx( Register d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stdbrx(Register d, Register s2){ illtrap(); } // This is a PPC instruction and should be removed

// ra0 version
inline void Assembler::icbi(    Register s2)          { illtrap(); } // This is a PPC instruction and should be removed
//inline void Assembler::dcba(  Register s2)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbz(    Register s2)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbst(   Register s2)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbf(    Register s2)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbt(    Register s2)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbtct(  Register s2, int ct)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbtds(  Register s2, int ds)  { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbtst(  Register s2)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::dcbtstct(Register s2, int ct)  { illtrap(); } // This is a PPC instruction and should be removed

// ra0 version
inline void Assembler::lbarx_unchecked(Register d, Register b, int eh1)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lharx_unchecked(Register d, Register b, int eh1)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwarx_unchecked(Register d, Register b, int eh1)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ldarx_unchecked(Register d, Register b, int eh1)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lqarx_unchecked(Register d, Register b, int eh1)          { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lbarx( Register d, Register b, bool hint_exclusive_access){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lharx( Register d, Register b, bool hint_exclusive_access){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lwarx( Register d, Register b, bool hint_exclusive_access){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::ldarx( Register d, Register b, bool hint_exclusive_access){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lqarx( Register d, Register b, bool hint_exclusive_access){ illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stbcx_(Register s, Register b)                            { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::sthcx_(Register s, Register b)                            { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stwcx_(Register s, Register b)                            { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stdcx_(Register s, Register b)                            { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stqcx_(Register s, Register b)                            { illtrap(); } // This is a PPC instruction and should be removed

// ra0 version
inline void Assembler::lfs( FloatRegister d, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lfsx(FloatRegister d, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lfd( FloatRegister d, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lfdx(FloatRegister d, Register b) { illtrap(); } // This is a PPC instruction and should be removed

// ra0 version
inline void Assembler::stfs( FloatRegister s, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stfsx(FloatRegister s, Register b) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stfd( FloatRegister s, int si16)   { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stfdx(FloatRegister s, Register b) { illtrap(); } // This is a PPC instruction and should be removed

// ra0 version
inline void Assembler::lvebx( VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvehx( VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvewx( VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvx(   VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvxl(  VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stvebx(VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stvehx(VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stvewx(VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stvx(  VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::stvxl( VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvsl(  VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed
inline void Assembler::lvsr(  VectorRegister d, Register s2) { illtrap(); } // This is a PPC instruction and should be removed

inline void Assembler::load_perm(VectorRegister perm, Register addr) {
#if defined(VM_LITTLE_ENDIAN)
  lvsr(perm, addr);
#else
  lvsl(perm, addr);
#endif
}

inline void Assembler::vec_perm(VectorRegister first_dest, VectorRegister second, VectorRegister perm) {
#if defined(VM_LITTLE_ENDIAN)
  vperm(first_dest, second, first_dest, perm);
#else
  vperm(first_dest, first_dest, second, perm);
#endif
}

inline void Assembler::vec_perm(VectorRegister dest, VectorRegister first, VectorRegister second, VectorRegister perm) {
#if defined(VM_LITTLE_ENDIAN)
  vperm(dest, second, first, perm);
#else
  vperm(dest, first, second, perm);
#endif
}

inline void Assembler::load_const(Register d, void* x, Register tmp) {
   load_const(d, (long)x, tmp);
}

// Load a 64 bit constant encoded by a `Label'. This works for bound
// labels as well as unbound ones. For unbound labels, the code will
// be patched as soon as the label gets bound.
inline void Assembler::load_const(Register d, Label& L, Register tmp) {
  load_const(d, target(L), tmp);
}

// Load a 64 bit constant encoded by an AddressLiteral. patchable.
inline void Assembler::load_const(Register d, AddressLiteral& a, Register tmp) {
  // First relocate (we don't change the offset in the RelocationHolder,
  // just pass a.rspec()), then delegate to load_const(Register, long).
  relocate(a.rspec());
  load_const(d, (long)a.value(), tmp);
}

inline void Assembler::load_const32(Register d, int i) {
  lis(d, i >> 16);
  ori(d, d, i & 0xFFFF);
}

#endif // CPU_RISCV_ASSEMBLER_RISCV_INLINE_HPP
