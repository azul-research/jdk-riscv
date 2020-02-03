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

#ifndef CPU_RISCV_ASSEMBLER_RISCV_HPP
#define CPU_RISCV_ASSEMBLER_RISCV_HPP

#include "asm/register.hpp"

// Address is an abstraction used to represent a memory location
// as used in assembler instructions.
// RISCV instructions grok either baseReg + indexReg or baseReg + disp.
class Address {
 private:
  Register _base;         // Base register.
  Register _index;        // Index register.
  intptr_t _disp;         // Displacement.

 public:
  Address(Register b, Register i, address d = 0)
    : _base(b), _index(i), _disp((intptr_t)d) {
    assert(i == noreg || d == 0, "can't have both");
  }

  Address(Register b, address d = 0)
    : _base(b), _index(noreg), _disp((intptr_t)d) {}

  Address(Register b, intptr_t d)
    : _base(b), _index(noreg), _disp(d) {}

  Address(Register b, RegisterOrConstant roc)
    : _base(b), _index(noreg), _disp(0) {
    if (roc.is_constant()) _disp = roc.as_constant(); else _index = roc.as_register();
  }

  Address()
    : _base(noreg), _index(noreg), _disp(0) {}

  // accessors
  Register base()  const { return _base; }
  Register index() const { return _index; }
  int      disp()  const { return (int)_disp; }
  bool     is_const() const { return _base == noreg && _index == noreg; }
};

class AddressLiteral {
 private:
  address          _address;
  RelocationHolder _rspec;

  RelocationHolder rspec_from_rtype(relocInfo::relocType rtype, address addr) {
    switch (rtype) {
    case relocInfo::external_word_type:
      return external_word_Relocation::spec(addr);
    case relocInfo::internal_word_type:
      return internal_word_Relocation::spec(addr);
    case relocInfo::opt_virtual_call_type:
      return opt_virtual_call_Relocation::spec();
    case relocInfo::static_call_type:
      return static_call_Relocation::spec();
    case relocInfo::runtime_call_type:
      return runtime_call_Relocation::spec();
    case relocInfo::none:
      return RelocationHolder();
    default:
      ShouldNotReachHere();
      return RelocationHolder();
    }
  }

 protected:
  // creation
  AddressLiteral() : _address(NULL), _rspec(NULL) {}

 public:
  AddressLiteral(address addr, RelocationHolder const& rspec)
    : _address(addr),
      _rspec(rspec) {}

  AddressLiteral(address addr, relocInfo::relocType rtype = relocInfo::none)
    : _address((address) addr),
      _rspec(rspec_from_rtype(rtype, (address) addr)) {}

  AddressLiteral(oop* addr, relocInfo::relocType rtype = relocInfo::none)
    : _address((address) addr),
      _rspec(rspec_from_rtype(rtype, (address) addr)) {}

  intptr_t value() const { return (intptr_t) _address; }

  const RelocationHolder& rspec() const { return _rspec; }
};

// Argument is an abstraction used to represent an outgoing
// actual argument or an incoming formal parameter, whether
// it resides in memory or in a register, in a manner consistent
// with the RISCV Application Binary Interface, or ABI. This is
// often referred to as the native or C calling convention.

class Argument {
 private:
  int _number;  // The number of the argument.
 public:
  enum {
    // Only 8 registers may contain integer parameters.
    n_register_parameters = 8,
    // Can have up to 8 floating registers.
    n_float_register_parameters = 8,

    // RISCV C calling conventions.
    // The first eight arguments are passed in int regs if they are int.
    n_int_register_parameters_c = 8,
    // The first thirteen float arguments are passed in float regs.
    n_float_register_parameters_c = 13,
    // Only the first 8 parameters are not placed on the stack. Aix disassembly
    // shows that xlC places all float args after argument 8 on the stack AND
    // in a register. This is not documented, but we follow this convention, too.
    n_regs_not_on_stack_c = 8,
  };
  // creation
  Argument(int number) : _number(number) {}

  int  number() const { return _number; }

  // Locating register-based arguments:
  bool is_register() const { return _number < n_register_parameters; }

  Register as_register() const {
    assert(is_register(), "must be a register argument");
    return as_Register(number() + R3_ARG1->encoding());
  }
};

#if !defined(ABI_ELFv2)
// A riscv64 function descriptor.
struct FunctionDescriptor {
 private:
  address _entry;
  address _toc;
  address _env;

 public:
  inline address entry() const { return _entry; }
  inline address toc()   const { return _toc; }
  inline address env()   const { return _env; }

  inline void set_entry(address entry) { _entry = entry; }
  inline void set_toc(  address toc)   { _toc   = toc; }
  inline void set_env(  address env)   { _env   = env; }

  inline static ByteSize entry_offset() { return byte_offset_of(FunctionDescriptor, _entry); }
  inline static ByteSize toc_offset()   { return byte_offset_of(FunctionDescriptor, _toc); }
  inline static ByteSize env_offset()   { return byte_offset_of(FunctionDescriptor, _env); }

  // Friend functions can be called without loading toc and env.
  enum {
    friend_toc = 0xcafe,
    friend_env = 0xc0de
  };

  inline bool is_friend_function() const {
    return (toc() == (address) friend_toc) && (env() == (address) friend_env);
  }

  // Constructor for stack-allocated instances.
  FunctionDescriptor() {
    _entry = (address) 0xbad;
    _toc   = (address) 0xbad;
    _env   = (address) 0xbad;
  }
};
#endif


// The RISCV Assembler: Pure assembler doing NO optimizations on the
// instruction level; i.e., what you write is what you get. The
// Assembler is generating code into a CodeBuffer.

class Assembler : public AbstractAssembler {
 protected:
  // Displacement routines
  static int  patched_branch(int dest_pos, int inst, int inst_pos);
  static int  branch_destination(int inst, int pos);

  friend class AbstractAssembler;

  // Code patchers need various routines like inv_wdisp()
  friend class NativeInstruction;
  friend class NativeGeneralJump;
  friend class Relocation;

 protected:
  static inline int rd(int x) { return x << 7; }
  static inline int rd(Register x) { return rd(x->encoding()); }
  static inline int rd(FloatRegister x) { return rd(x->encoding()); }
  static inline int funct2(int x) { return x << 25; }
  static inline int funct3(int x) { return x << 12; }
  static inline int funct7(int x) { return (int)((unsigned)x << 25); }
  static inline int funct7(int x, bool aq, bool rl) { return (int)(((unsigned)x << 27) | (aq << 26) | (rl << 25)); }
  static inline int rs1(int x) { return x << 15; }
  static inline int rs1(Register x) { return rs1(x->encoding()); }
  static inline int rs1(FloatRegister x) { return rs1(x->encoding()); }
  static inline int rs2(int x) { return x << 20; }
  static inline int rs2(Register x) { return rs2(x->encoding()); }
  static inline int rs2(FloatRegister x) { return rs2(x->encoding()); }
  static inline int rs3(int x) { return x << 27; }
  static inline int rs3(Register x) { return rs3(x->encoding()); }

  // TODO_RISCV: more asserts for immX() functions?
  static inline int immi(int x) { return (int)((unsigned)x << 20); }
  static inline int imms(int x) { return (int)(((x & 0x1f) << 7) | ((unsigned)x >> 5 << 25)); }
  static inline int immb(int x) {
    assert((x & 1) == 0, "B-immediate must be zero in bit 0");
    return (int)(
        ((x & 0x1e) << 7) | (((x >> 5) & 0x3f) << 25) |
        ((unsigned)(x >> 12) << 31) | (((x >> 11) & 0x1) << 7)
    );
  }
  static inline int immu(int x) {
    assert((x & 0xfff) == 0, "U-immediate must be zero in bits 0:11");
    return x & 0xfffff000;
  }
  static inline int immj(int x) {
    assert((x & 1) == 0, "J-immediate must be zero in bit 0");
    return (int)(
        ((x & 0x7fe) << 20) | ((x & 0x800) << 9) |
        (x & 0xff000) | ((unsigned)(x & 0x100000) << 11)
    );
  }

 public:
  enum rounding_mode {
    RNE = 0x0,
    RTZ = 0x1,
    RDN = 0x2,
    RUP = 0x3,
    RMM = 0x4,
    DYN = 0x7
  };

  enum shifts {
    XO_21_29_SHIFT = 2,
    XO_21_30_SHIFT = 1,
    XO_27_29_SHIFT = 2,
    XO_30_31_SHIFT = 0,
    SPR_5_9_SHIFT  = 11u, // SPR_5_9 field in bits 11 -- 15
    SPR_0_4_SHIFT  = 16u, // SPR_0_4 field in bits 16 -- 20
    RS_SHIFT       = 21u, // RS field in bits 21 -- 25
    OPCODE_SHIFT   = 26u, // opcode in bits 26 -- 31
  };

  enum opcdxos_masks {
    XL_FORM_OPCODE_MASK = (63u << OPCODE_SHIFT) | (1023u << 1),
    ADDI_OPCODE_MASK    = (63u << OPCODE_SHIFT),
    ADDIS_OPCODE_MASK   = (63u << OPCODE_SHIFT),
    BXX_OPCODE_MASK     = (63u << OPCODE_SHIFT),
    BCXX_OPCODE_MASK    = (63u << OPCODE_SHIFT),
    // trap instructions
    TDI_OPCODE_MASK     = (63u << OPCODE_SHIFT),
    TWI_OPCODE_MASK     = (63u << OPCODE_SHIFT),
    TD_OPCODE_MASK      = (63u << OPCODE_SHIFT) | (1023u << 1),
    TW_OPCODE_MASK      = (63u << OPCODE_SHIFT) | (1023u << 1),
    LD_OPCODE_MASK      = (63u << OPCODE_SHIFT) | (3u << XO_30_31_SHIFT), // DS-FORM
    STD_OPCODE_MASK     = LD_OPCODE_MASK,
    STDU_OPCODE_MASK    = STD_OPCODE_MASK,
    STDX_OPCODE_MASK    = (63u << OPCODE_SHIFT) | (1023u << 1),
    STDUX_OPCODE_MASK   = STDX_OPCODE_MASK,
    STW_OPCODE_MASK     = (63u << OPCODE_SHIFT),
    STWU_OPCODE_MASK    = STW_OPCODE_MASK,
    STWX_OPCODE_MASK    = (63u << OPCODE_SHIFT) | (1023u << 1),
    STWUX_OPCODE_MASK   = STWX_OPCODE_MASK,
    MTCTR_OPCODE_MASK   = ~(31u << RS_SHIFT),
    ORI_OPCODE_MASK     = (63u << OPCODE_SHIFT),
    ORIS_OPCODE_MASK    = (63u << OPCODE_SHIFT),
    RLDICR_OPCODE_MASK  = (63u << OPCODE_SHIFT) | (7u << XO_27_29_SHIFT)
  };

  enum opcdxos {
    // RISCV base opcodes, suffixed with RV_OPCODE temporary, as to not
    // to conflict with PPC's instructions.
    //
    // From https://riscv.org/specifications/ beginning of
    // Chapter 25. RV32/64G Instruction Set Listings (Table 25.1)

    LOAD_RV_OPCODE = 0x03,
    LOAD_FP_RV_OPCODE = 0x07,
    MISC_MEM_RV_OPCODE = 0x0f,
    OP_IMM_RV_OPCODE = 0x13,
    AUIPC_RV_OPCODE = 0x17,
    OP_IMM32_RV_OPCODE = 0x1b,
    STORE_RV_OPCODE = 0x23,
    STORE_FP_RV_OPCODE = 0x27,
    AMO_RV_OPCODE = 0x2f,
    OP_RV_OPCODE = 0x33,
    LUI_RV_OPCODE = 0x37,
    OP32_RV_OPCODE = 0x3b,
    MADD_RV_OPCODE = 0x43,
    MSUB_RV_OPCODE = 0x47,
    NMSUB_RV_OPCODE = 0x4b,
    NMADD_RV_OPCODE = 0x4f,
    OP_FP_RV_OPCODE = 0x53,
    BRANCH_RV_OPCODE = 0x63,
    JALR_RV_OPCODE = 0x67,
    JAL_RV_OPCODE = 0x6f,
    SYSTEM_RV_OPCODE = 0x73,

    // PPC opcodes follow:
    ADD_OPCODE    = (31u << OPCODE_SHIFT | 266u << 1),
    ADDC_OPCODE   = (31u << OPCODE_SHIFT |  10u << 1),
    ADDI_OPCODE   = (14u << OPCODE_SHIFT),
    ADDIS_OPCODE  = (15u << OPCODE_SHIFT),
    ADDIC__OPCODE = (13u << OPCODE_SHIFT),
    ADDE_OPCODE   = (31u << OPCODE_SHIFT | 138u << 1),
    ADDME_OPCODE  = (31u << OPCODE_SHIFT | 234u << 1),
    ADDZE_OPCODE  = (31u << OPCODE_SHIFT | 202u << 1),
    SUBF_OPCODE   = (31u << OPCODE_SHIFT |  40u << 1),
    SUBFC_OPCODE  = (31u << OPCODE_SHIFT |   8u << 1),
    SUBFE_OPCODE  = (31u << OPCODE_SHIFT | 136u << 1),
    SUBFIC_OPCODE = (8u  << OPCODE_SHIFT),
    SUBFME_OPCODE = (31u << OPCODE_SHIFT | 232u << 1),
    SUBFZE_OPCODE = (31u << OPCODE_SHIFT | 200u << 1),
    DIVW_OPCODE   = (31u << OPCODE_SHIFT | 491u << 1),
    MULLW_OPCODE  = (31u << OPCODE_SHIFT | 235u << 1),
    MULHW_OPCODE  = (31u << OPCODE_SHIFT |  75u << 1),
    MULHWU_OPCODE = (31u << OPCODE_SHIFT |  11u << 1),
    MULLI_OPCODE  = (7u  << OPCODE_SHIFT),
    AND_OPCODE    = (31u << OPCODE_SHIFT |  28u << 1),
    ANDI_OPCODE   = (28u << OPCODE_SHIFT),
    ANDIS_OPCODE  = (29u << OPCODE_SHIFT),
    ANDC_OPCODE   = (31u << OPCODE_SHIFT |  60u << 1),
    ORC_OPCODE    = (31u << OPCODE_SHIFT | 412u << 1),
    OR_OPCODE     = (31u << OPCODE_SHIFT | 444u << 1),
    ORI_OPCODE    = (24u << OPCODE_SHIFT),
    ORIS_OPCODE   = (25u << OPCODE_SHIFT),
    XOR_OPCODE    = (31u << OPCODE_SHIFT | 316u << 1),
    XORI_OPCODE   = (26u << OPCODE_SHIFT),
    XORIS_OPCODE  = (27u << OPCODE_SHIFT),

    NEG_OPCODE    = (31u << OPCODE_SHIFT | 104u << 1),

    RLWINM_OPCODE = (21u << OPCODE_SHIFT),
    CLRRWI_OPCODE = RLWINM_OPCODE,
    CLRLWI_OPCODE = RLWINM_OPCODE,

    RLWIMI_OPCODE = (20u << OPCODE_SHIFT),

    SLW_OPCODE    = (31u << OPCODE_SHIFT |  24u << 1),
    SLWI_OPCODE   = RLWINM_OPCODE,
    SRW_OPCODE    = (31u << OPCODE_SHIFT | 536u << 1),
    SRWI_OPCODE   = RLWINM_OPCODE,
    SRAW_OPCODE   = (31u << OPCODE_SHIFT | 792u << 1),
    SRAWI_OPCODE  = (31u << OPCODE_SHIFT | 824u << 1),

    CMP_OPCODE    = (31u << OPCODE_SHIFT |   0u << 1),
    CMPI_OPCODE   = (11u << OPCODE_SHIFT),
    CMPL_OPCODE   = (31u << OPCODE_SHIFT |  32u << 1),
    CMPLI_OPCODE  = (10u << OPCODE_SHIFT),
    CMPRB_OPCODE  = (31u << OPCODE_SHIFT | 192u << 1),
    CMPEQB_OPCODE = (31u << OPCODE_SHIFT | 224u << 1),

    ISEL_OPCODE   = (31u << OPCODE_SHIFT |  15u << 1),

    // Special purpose registers
    MTSPR_OPCODE  = (31u << OPCODE_SHIFT | 467u << 1),
    MFSPR_OPCODE  = (31u << OPCODE_SHIFT | 339u << 1),

    MTXER_OPCODE  = (MTSPR_OPCODE | 1 << SPR_0_4_SHIFT),
    MFXER_OPCODE  = (MFSPR_OPCODE | 1 << SPR_0_4_SHIFT),

    MTDSCR_OPCODE = (MTSPR_OPCODE | 3 << SPR_0_4_SHIFT),
    MFDSCR_OPCODE = (MFSPR_OPCODE | 3 << SPR_0_4_SHIFT),

    MTLR_OPCODE   = (MTSPR_OPCODE | 8 << SPR_0_4_SHIFT),
    MFLR_OPCODE   = (MFSPR_OPCODE | 8 << SPR_0_4_SHIFT),

    MTCTR_OPCODE  = (MTSPR_OPCODE | 9 << SPR_0_4_SHIFT),
    MFCTR_OPCODE  = (MFSPR_OPCODE | 9 << SPR_0_4_SHIFT),

    // Attention: Higher and lower half are inserted in reversed order.
    MTTFHAR_OPCODE   = (MTSPR_OPCODE | 4 << SPR_5_9_SHIFT | 0 << SPR_0_4_SHIFT),
    MFTFHAR_OPCODE   = (MFSPR_OPCODE | 4 << SPR_5_9_SHIFT | 0 << SPR_0_4_SHIFT),
    MTTFIAR_OPCODE   = (MTSPR_OPCODE | 4 << SPR_5_9_SHIFT | 1 << SPR_0_4_SHIFT),
    MFTFIAR_OPCODE   = (MFSPR_OPCODE | 4 << SPR_5_9_SHIFT | 1 << SPR_0_4_SHIFT),
    MTTEXASR_OPCODE  = (MTSPR_OPCODE | 4 << SPR_5_9_SHIFT | 2 << SPR_0_4_SHIFT),
    MFTEXASR_OPCODE  = (MFSPR_OPCODE | 4 << SPR_5_9_SHIFT | 2 << SPR_0_4_SHIFT),
    MTTEXASRU_OPCODE = (MTSPR_OPCODE | 4 << SPR_5_9_SHIFT | 3 << SPR_0_4_SHIFT),
    MFTEXASRU_OPCODE = (MFSPR_OPCODE | 4 << SPR_5_9_SHIFT | 3 << SPR_0_4_SHIFT),

    MTVRSAVE_OPCODE  = (MTSPR_OPCODE | 8 << SPR_5_9_SHIFT | 0 << SPR_0_4_SHIFT),
    MFVRSAVE_OPCODE  = (MFSPR_OPCODE | 8 << SPR_5_9_SHIFT | 0 << SPR_0_4_SHIFT),

    MFTB_OPCODE   = (MFSPR_OPCODE | 8 << SPR_5_9_SHIFT | 12 << SPR_0_4_SHIFT),

    MTCRF_OPCODE  = (31u << OPCODE_SHIFT | 144u << 1),
    MFCR_OPCODE   = (31u << OPCODE_SHIFT | 19u << 1),
    MCRF_OPCODE   = (19u << OPCODE_SHIFT | 0u << 1),
    SETB_OPCODE   = (31u << OPCODE_SHIFT | 128u << 1),

    // condition register logic instructions
    CRAND_OPCODE  = (19u << OPCODE_SHIFT | 257u << 1),
    CRNAND_OPCODE = (19u << OPCODE_SHIFT | 225u << 1),
    CROR_OPCODE   = (19u << OPCODE_SHIFT | 449u << 1),
    CRXOR_OPCODE  = (19u << OPCODE_SHIFT | 193u << 1),
    CRNOR_OPCODE  = (19u << OPCODE_SHIFT |  33u << 1),
    CREQV_OPCODE  = (19u << OPCODE_SHIFT | 289u << 1),
    CRANDC_OPCODE = (19u << OPCODE_SHIFT | 129u << 1),
    CRORC_OPCODE  = (19u << OPCODE_SHIFT | 417u << 1),

    BCLR_OPCODE   = (19u << OPCODE_SHIFT | 16u << 1),
    BXX_OPCODE      = (18u << OPCODE_SHIFT),
    BCXX_OPCODE     = (16u << OPCODE_SHIFT),

    // CTR-related opcodes
    BCCTR_OPCODE  = (19u << OPCODE_SHIFT | 528u << 1),

    LWZ_OPCODE   = (32u << OPCODE_SHIFT),
    LWZX_OPCODE  = (31u << OPCODE_SHIFT |  23u << 1),
    LWZU_OPCODE  = (33u << OPCODE_SHIFT),
    LWBRX_OPCODE = (31u << OPCODE_SHIFT |  534 << 1),

    LHA_OPCODE   = (42u << OPCODE_SHIFT),
    LHAX_OPCODE  = (31u << OPCODE_SHIFT | 343u << 1),
    LHAU_OPCODE  = (43u << OPCODE_SHIFT),

    LHZ_OPCODE   = (40u << OPCODE_SHIFT),
    LHZX_OPCODE  = (31u << OPCODE_SHIFT | 279u << 1),
    LHZU_OPCODE  = (41u << OPCODE_SHIFT),
    LHBRX_OPCODE = (31u << OPCODE_SHIFT |  790 << 1),

    LBZ_OPCODE   = (34u << OPCODE_SHIFT),
    LBZX_OPCODE  = (31u << OPCODE_SHIFT |  87u << 1),
    LBZU_OPCODE  = (35u << OPCODE_SHIFT),

    STW_OPCODE   = (36u << OPCODE_SHIFT),
    STWX_OPCODE  = (31u << OPCODE_SHIFT | 151u << 1),
    STWU_OPCODE  = (37u << OPCODE_SHIFT),
    STWUX_OPCODE = (31u << OPCODE_SHIFT | 183u << 1),
    STWBRX_OPCODE = (31u << OPCODE_SHIFT | 662u << 1),

    STH_OPCODE   = (44u << OPCODE_SHIFT),
    STHX_OPCODE  = (31u << OPCODE_SHIFT | 407u << 1),
    STHU_OPCODE  = (45u << OPCODE_SHIFT),
    STHBRX_OPCODE = (31u << OPCODE_SHIFT | 918u << 1),

    STB_OPCODE   = (38u << OPCODE_SHIFT),
    STBX_OPCODE  = (31u << OPCODE_SHIFT | 215u << 1),
    STBU_OPCODE  = (39u << OPCODE_SHIFT),

    EXTSB_OPCODE = (31u << OPCODE_SHIFT | 954u << 1),
    EXTSH_OPCODE = (31u << OPCODE_SHIFT | 922u << 1),
    EXTSW_OPCODE = (31u << OPCODE_SHIFT | 986u << 1),               // X-FORM

    // 32 bit opcode encodings

    LWA_OPCODE    = (58u << OPCODE_SHIFT |   2u << XO_30_31_SHIFT), // DS-FORM
    LWAX_OPCODE   = (31u << OPCODE_SHIFT | 341u << XO_21_30_SHIFT), // X-FORM

    CNTLZW_OPCODE = (31u << OPCODE_SHIFT |  26u << XO_21_30_SHIFT), // X-FORM
    CNTTZW_OPCODE = (31u << OPCODE_SHIFT | 538u << XO_21_30_SHIFT), // X-FORM

    // 64 bit opcode encodings

    LD_OPCODE     = (58u << OPCODE_SHIFT |   0u << XO_30_31_SHIFT), // DS-FORM
    LDU_OPCODE    = (58u << OPCODE_SHIFT |   1u << XO_30_31_SHIFT), // DS-FORM
    LDX_OPCODE    = (31u << OPCODE_SHIFT |  21u << XO_21_30_SHIFT), // X-FORM
    LDBRX_OPCODE  = (31u << OPCODE_SHIFT | 532u << 1),              // X-FORM

    STD_OPCODE    = (62u << OPCODE_SHIFT |   0u << XO_30_31_SHIFT), // DS-FORM
    STDU_OPCODE   = (62u << OPCODE_SHIFT |   1u << XO_30_31_SHIFT), // DS-FORM
    STDUX_OPCODE  = (31u << OPCODE_SHIFT | 181u << 1),              // X-FORM
    STDX_OPCODE   = (31u << OPCODE_SHIFT | 149u << XO_21_30_SHIFT), // X-FORM
    STDBRX_OPCODE = (31u << OPCODE_SHIFT | 660u << 1),              // X-FORM

    RLDICR_OPCODE = (30u << OPCODE_SHIFT |   1u << XO_27_29_SHIFT), // MD-FORM
    RLDICL_OPCODE = (30u << OPCODE_SHIFT |   0u << XO_27_29_SHIFT), // MD-FORM
    RLDIC_OPCODE  = (30u << OPCODE_SHIFT |   2u << XO_27_29_SHIFT), // MD-FORM
    RLDIMI_OPCODE = (30u << OPCODE_SHIFT |   3u << XO_27_29_SHIFT), // MD-FORM

    SRADI_OPCODE  = (31u << OPCODE_SHIFT | 413u << XO_21_29_SHIFT), // XS-FORM

    SLD_OPCODE    = (31u << OPCODE_SHIFT |  27u << 1),              // X-FORM
    SRD_OPCODE    = (31u << OPCODE_SHIFT | 539u << 1),              // X-FORM
    SRAD_OPCODE   = (31u << OPCODE_SHIFT | 794u << 1),              // X-FORM

    MULLD_OPCODE  = (31u << OPCODE_SHIFT | 233u << 1),              // XO-FORM
    MULHD_OPCODE  = (31u << OPCODE_SHIFT |  73u << 1),              // XO-FORM
    MULHDU_OPCODE = (31u << OPCODE_SHIFT |   9u << 1),              // XO-FORM
    DIVD_OPCODE   = (31u << OPCODE_SHIFT | 489u << 1),              // XO-FORM

    CNTLZD_OPCODE = (31u << OPCODE_SHIFT |  58u << XO_21_30_SHIFT), // X-FORM
    CNTTZD_OPCODE = (31u << OPCODE_SHIFT | 570u << XO_21_30_SHIFT), // X-FORM
    NAND_OPCODE   = (31u << OPCODE_SHIFT | 476u << XO_21_30_SHIFT), // X-FORM
    NOR_OPCODE    = (31u << OPCODE_SHIFT | 124u << XO_21_30_SHIFT), // X-FORM


    // opcodes only used for floating arithmetic
    FADD_OPCODE   = (63u << OPCODE_SHIFT |  21u << 1),
    FADDS_OPCODE  = (59u << OPCODE_SHIFT |  21u << 1),
    FCMPU_OPCODE  = (63u << OPCODE_SHIFT |  00u << 1),
    FDIV_OPCODE   = (63u << OPCODE_SHIFT |  18u << 1),
    FDIVS_OPCODE  = (59u << OPCODE_SHIFT |  18u << 1),
    FMR_OPCODE    = (63u << OPCODE_SHIFT |  72u << 1),
    // These are special Power6 opcodes, reused for "lfdepx" and "stfdepx"
    // on Power7.  Do not use.
    // MFFGPR_OPCODE  = (31u << OPCODE_SHIFT | 607u << 1),
    // MFTGPR_OPCODE  = (31u << OPCODE_SHIFT | 735u << 1),
    CMPB_OPCODE    = (31u << OPCODE_SHIFT |  508  << 1),
    POPCNTB_OPCODE = (31u << OPCODE_SHIFT |  122  << 1),
    POPCNTW_OPCODE = (31u << OPCODE_SHIFT |  378  << 1),
    POPCNTD_OPCODE = (31u << OPCODE_SHIFT |  506  << 1),
    FABS_OPCODE    = (63u << OPCODE_SHIFT |  264u << 1),
    FNABS_OPCODE   = (63u << OPCODE_SHIFT |  136u << 1),
    FMUL_OPCODE    = (63u << OPCODE_SHIFT |   25u << 1),
    FMULS_OPCODE   = (59u << OPCODE_SHIFT |   25u << 1),
    FNEG_OPCODE    = (63u << OPCODE_SHIFT |   40u << 1),
    FSUB_OPCODE    = (63u << OPCODE_SHIFT |   20u << 1),
    FSUBS_OPCODE   = (59u << OPCODE_SHIFT |   20u << 1),

    // RISCV64-internal FPU conversion opcodes
    FCFID_OPCODE   = (63u << OPCODE_SHIFT |  846u << 1),
    FCFIDS_OPCODE  = (59u << OPCODE_SHIFT |  846u << 1),
    FCTID_OPCODE   = (63u << OPCODE_SHIFT |  814u << 1),
    FCTIDZ_OPCODE  = (63u << OPCODE_SHIFT |  815u << 1),
    FCTIW_OPCODE   = (63u << OPCODE_SHIFT |   14u << 1),
    FCTIWZ_OPCODE  = (63u << OPCODE_SHIFT |   15u << 1),
    FRSP_OPCODE    = (63u << OPCODE_SHIFT |   12u << 1),

    // Fused multiply-accumulate instructions.
    FMADD_OPCODE   = (63u << OPCODE_SHIFT |   29u << 1),
    FMADDS_OPCODE  = (59u << OPCODE_SHIFT |   29u << 1),
    FMSUB_OPCODE   = (63u << OPCODE_SHIFT |   28u << 1),
    FMSUBS_OPCODE  = (59u << OPCODE_SHIFT |   28u << 1),
    FNMADD_OPCODE  = (63u << OPCODE_SHIFT |   31u << 1),
    FNMADDS_OPCODE = (59u << OPCODE_SHIFT |   31u << 1),
    FNMSUB_OPCODE  = (63u << OPCODE_SHIFT |   30u << 1),
    FNMSUBS_OPCODE = (59u << OPCODE_SHIFT |   30u << 1),

    LFD_OPCODE     = (50u << OPCODE_SHIFT |   00u << 1),
    LFDU_OPCODE    = (51u << OPCODE_SHIFT |   00u << 1),
    LFDX_OPCODE    = (31u << OPCODE_SHIFT |  599u << 1),
    LFS_OPCODE     = (48u << OPCODE_SHIFT |   00u << 1),
    LFSU_OPCODE    = (49u << OPCODE_SHIFT |   00u << 1),
    LFSX_OPCODE    = (31u << OPCODE_SHIFT |  535u << 1),

    STFD_OPCODE    = (54u << OPCODE_SHIFT |   00u << 1),
    STFDU_OPCODE   = (55u << OPCODE_SHIFT |   00u << 1),
    STFDX_OPCODE   = (31u << OPCODE_SHIFT |  727u << 1),
    STFS_OPCODE    = (52u << OPCODE_SHIFT |   00u << 1),
    STFSU_OPCODE   = (53u << OPCODE_SHIFT |   00u << 1),
    STFSX_OPCODE   = (31u << OPCODE_SHIFT |  663u << 1),

    FSQRT_OPCODE   = (63u << OPCODE_SHIFT |   22u << 1),            // A-FORM
    FSQRTS_OPCODE  = (59u << OPCODE_SHIFT |   22u << 1),            // A-FORM

    // Vector instruction support for >= Power6
    // Vector Storage Access
    LVEBX_OPCODE   = (31u << OPCODE_SHIFT |    7u << 1),
    LVEHX_OPCODE   = (31u << OPCODE_SHIFT |   39u << 1),
    LVEWX_OPCODE   = (31u << OPCODE_SHIFT |   71u << 1),
    LVX_OPCODE     = (31u << OPCODE_SHIFT |  103u << 1),
    LVXL_OPCODE    = (31u << OPCODE_SHIFT |  359u << 1),
    STVEBX_OPCODE  = (31u << OPCODE_SHIFT |  135u << 1),
    STVEHX_OPCODE  = (31u << OPCODE_SHIFT |  167u << 1),
    STVEWX_OPCODE  = (31u << OPCODE_SHIFT |  199u << 1),
    STVX_OPCODE    = (31u << OPCODE_SHIFT |  231u << 1),
    STVXL_OPCODE   = (31u << OPCODE_SHIFT |  487u << 1),
    LVSL_OPCODE    = (31u << OPCODE_SHIFT |    6u << 1),
    LVSR_OPCODE    = (31u << OPCODE_SHIFT |   38u << 1),

    // Vector-Scalar (VSX) instruction support.
    LXVD2X_OPCODE  = (31u << OPCODE_SHIFT |  844u << 1),
    STXVD2X_OPCODE = (31u << OPCODE_SHIFT |  972u << 1),
    MTVSRD_OPCODE  = (31u << OPCODE_SHIFT |  179u << 1),
    MTVSRWZ_OPCODE = (31u << OPCODE_SHIFT |  243u << 1),
    MFVSRD_OPCODE  = (31u << OPCODE_SHIFT |   51u << 1),
    MTVSRWA_OPCODE = (31u << OPCODE_SHIFT |  211u << 1),
    MFVSRWZ_OPCODE = (31u << OPCODE_SHIFT |  115u << 1),
    XXPERMDI_OPCODE= (60u << OPCODE_SHIFT |   10u << 3),
    XXMRGHW_OPCODE = (60u << OPCODE_SHIFT |   18u << 3),
    XXMRGLW_OPCODE = (60u << OPCODE_SHIFT |   50u << 3),
    XXSPLTW_OPCODE = (60u << OPCODE_SHIFT |  164u << 2),
    XXLOR_OPCODE   = (60u << OPCODE_SHIFT |  146u << 3),
    XXLXOR_OPCODE  = (60u << OPCODE_SHIFT |  154u << 3),
    XXLEQV_OPCODE  = (60u << OPCODE_SHIFT |  186u << 3),
    XVDIVSP_OPCODE = (60u << OPCODE_SHIFT |   88u << 3),
    XVDIVDP_OPCODE = (60u << OPCODE_SHIFT |  120u << 3),
    XVABSSP_OPCODE = (60u << OPCODE_SHIFT |  409u << 2),
    XVABSDP_OPCODE = (60u << OPCODE_SHIFT |  473u << 2),
    XVNEGSP_OPCODE = (60u << OPCODE_SHIFT |  441u << 2),
    XVNEGDP_OPCODE = (60u << OPCODE_SHIFT |  505u << 2),
    XVSQRTSP_OPCODE= (60u << OPCODE_SHIFT |  139u << 2),
    XVSQRTDP_OPCODE= (60u << OPCODE_SHIFT |  203u << 2),
    XSCVDPSPN_OPCODE=(60u << OPCODE_SHIFT |  267u << 2),
    XVADDDP_OPCODE = (60u << OPCODE_SHIFT |   96u << 3),
    XVSUBDP_OPCODE = (60u << OPCODE_SHIFT |  104u << 3),
    XVMULSP_OPCODE = (60u << OPCODE_SHIFT |   80u << 3),
    XVMULDP_OPCODE = (60u << OPCODE_SHIFT |  112u << 3),
    XVMADDASP_OPCODE=(60u << OPCODE_SHIFT |   65u << 3),
    XVMADDADP_OPCODE=(60u << OPCODE_SHIFT |   97u << 3),
    XVMSUBASP_OPCODE=(60u << OPCODE_SHIFT |   81u << 3),
    XVMSUBADP_OPCODE=(60u << OPCODE_SHIFT |  113u << 3),
    XVNMSUBASP_OPCODE=(60u<< OPCODE_SHIFT |  209u << 3),
    XVNMSUBADP_OPCODE=(60u<< OPCODE_SHIFT |  241u << 3),

    // Deliver A Random Number (introduced with POWER9)
    DARN_OPCODE    = (31u << OPCODE_SHIFT |  755u << 1),

    // Vector Permute and Formatting
    VPKPX_OPCODE   = (4u  << OPCODE_SHIFT |  782u     ),
    VPKSHSS_OPCODE = (4u  << OPCODE_SHIFT |  398u     ),
    VPKSWSS_OPCODE = (4u  << OPCODE_SHIFT |  462u     ),
    VPKSHUS_OPCODE = (4u  << OPCODE_SHIFT |  270u     ),
    VPKSWUS_OPCODE = (4u  << OPCODE_SHIFT |  334u     ),
    VPKUHUM_OPCODE = (4u  << OPCODE_SHIFT |   14u     ),
    VPKUWUM_OPCODE = (4u  << OPCODE_SHIFT |   78u     ),
    VPKUHUS_OPCODE = (4u  << OPCODE_SHIFT |  142u     ),
    VPKUWUS_OPCODE = (4u  << OPCODE_SHIFT |  206u     ),
    VUPKHPX_OPCODE = (4u  << OPCODE_SHIFT |  846u     ),
    VUPKHSB_OPCODE = (4u  << OPCODE_SHIFT |  526u     ),
    VUPKHSH_OPCODE = (4u  << OPCODE_SHIFT |  590u     ),
    VUPKLPX_OPCODE = (4u  << OPCODE_SHIFT |  974u     ),
    VUPKLSB_OPCODE = (4u  << OPCODE_SHIFT |  654u     ),
    VUPKLSH_OPCODE = (4u  << OPCODE_SHIFT |  718u     ),

    VMRGHB_OPCODE  = (4u  << OPCODE_SHIFT |   12u     ),
    VMRGHW_OPCODE  = (4u  << OPCODE_SHIFT |  140u     ),
    VMRGHH_OPCODE  = (4u  << OPCODE_SHIFT |   76u     ),
    VMRGLB_OPCODE  = (4u  << OPCODE_SHIFT |  268u     ),
    VMRGLW_OPCODE  = (4u  << OPCODE_SHIFT |  396u     ),
    VMRGLH_OPCODE  = (4u  << OPCODE_SHIFT |  332u     ),

    VSPLT_OPCODE   = (4u  << OPCODE_SHIFT |  524u     ),
    VSPLTH_OPCODE  = (4u  << OPCODE_SHIFT |  588u     ),
    VSPLTW_OPCODE  = (4u  << OPCODE_SHIFT |  652u     ),
    VSPLTISB_OPCODE= (4u  << OPCODE_SHIFT |  780u     ),
    VSPLTISH_OPCODE= (4u  << OPCODE_SHIFT |  844u     ),
    VSPLTISW_OPCODE= (4u  << OPCODE_SHIFT |  908u     ),

    VPERM_OPCODE   = (4u  << OPCODE_SHIFT |   43u     ),
    VSEL_OPCODE    = (4u  << OPCODE_SHIFT |   42u     ),

    VSL_OPCODE     = (4u  << OPCODE_SHIFT |  452u     ),
    VSLDOI_OPCODE  = (4u  << OPCODE_SHIFT |   44u     ),
    VSLO_OPCODE    = (4u  << OPCODE_SHIFT | 1036u     ),
    VSR_OPCODE     = (4u  << OPCODE_SHIFT |  708u     ),
    VSRO_OPCODE    = (4u  << OPCODE_SHIFT | 1100u     ),

    // Vector Integer
    VADDCUW_OPCODE = (4u  << OPCODE_SHIFT |  384u     ),
    VADDSHS_OPCODE = (4u  << OPCODE_SHIFT |  832u     ),
    VADDSBS_OPCODE = (4u  << OPCODE_SHIFT |  768u     ),
    VADDSWS_OPCODE = (4u  << OPCODE_SHIFT |  896u     ),
    VADDUBM_OPCODE = (4u  << OPCODE_SHIFT |    0u     ),
    VADDUWM_OPCODE = (4u  << OPCODE_SHIFT |  128u     ),
    VADDUHM_OPCODE = (4u  << OPCODE_SHIFT |   64u     ),
    VADDUDM_OPCODE = (4u  << OPCODE_SHIFT |  192u     ),
    VADDUBS_OPCODE = (4u  << OPCODE_SHIFT |  512u     ),
    VADDUWS_OPCODE = (4u  << OPCODE_SHIFT |  640u     ),
    VADDUHS_OPCODE = (4u  << OPCODE_SHIFT |  576u     ),
    VADDFP_OPCODE  = (4u  << OPCODE_SHIFT |   10u     ),
    VSUBCUW_OPCODE = (4u  << OPCODE_SHIFT | 1408u     ),
    VSUBSHS_OPCODE = (4u  << OPCODE_SHIFT | 1856u     ),
    VSUBSBS_OPCODE = (4u  << OPCODE_SHIFT | 1792u     ),
    VSUBSWS_OPCODE = (4u  << OPCODE_SHIFT | 1920u     ),
    VSUBUBM_OPCODE = (4u  << OPCODE_SHIFT | 1024u     ),
    VSUBUWM_OPCODE = (4u  << OPCODE_SHIFT | 1152u     ),
    VSUBUHM_OPCODE = (4u  << OPCODE_SHIFT | 1088u     ),
    VSUBUDM_OPCODE = (4u  << OPCODE_SHIFT | 1216u     ),
    VSUBUBS_OPCODE = (4u  << OPCODE_SHIFT | 1536u     ),
    VSUBUWS_OPCODE = (4u  << OPCODE_SHIFT | 1664u     ),
    VSUBUHS_OPCODE = (4u  << OPCODE_SHIFT | 1600u     ),
    VSUBFP_OPCODE  = (4u  << OPCODE_SHIFT |   74u     ),

    VMULESB_OPCODE = (4u  << OPCODE_SHIFT |  776u     ),
    VMULEUB_OPCODE = (4u  << OPCODE_SHIFT |  520u     ),
    VMULESH_OPCODE = (4u  << OPCODE_SHIFT |  840u     ),
    VMULEUH_OPCODE = (4u  << OPCODE_SHIFT |  584u     ),
    VMULOSB_OPCODE = (4u  << OPCODE_SHIFT |  264u     ),
    VMULOUB_OPCODE = (4u  << OPCODE_SHIFT |    8u     ),
    VMULOSH_OPCODE = (4u  << OPCODE_SHIFT |  328u     ),
    VMULOSW_OPCODE = (4u  << OPCODE_SHIFT |  392u     ),
    VMULOUH_OPCODE = (4u  << OPCODE_SHIFT |   72u     ),
    VMULUWM_OPCODE = (4u  << OPCODE_SHIFT |  137u     ),
    VMHADDSHS_OPCODE=(4u  << OPCODE_SHIFT |   32u     ),
    VMHRADDSHS_OPCODE=(4u << OPCODE_SHIFT |   33u     ),
    VMLADDUHM_OPCODE=(4u  << OPCODE_SHIFT |   34u     ),
    VMSUBUHM_OPCODE= (4u  << OPCODE_SHIFT |   36u     ),
    VMSUMMBM_OPCODE= (4u  << OPCODE_SHIFT |   37u     ),
    VMSUMSHM_OPCODE= (4u  << OPCODE_SHIFT |   40u     ),
    VMSUMSHS_OPCODE= (4u  << OPCODE_SHIFT |   41u     ),
    VMSUMUHM_OPCODE= (4u  << OPCODE_SHIFT |   38u     ),
    VMSUMUHS_OPCODE= (4u  << OPCODE_SHIFT |   39u     ),
    VMADDFP_OPCODE = (4u  << OPCODE_SHIFT |   46u     ),

    VSUMSWS_OPCODE = (4u  << OPCODE_SHIFT | 1928u     ),
    VSUM2SWS_OPCODE= (4u  << OPCODE_SHIFT | 1672u     ),
    VSUM4SBS_OPCODE= (4u  << OPCODE_SHIFT | 1800u     ),
    VSUM4UBS_OPCODE= (4u  << OPCODE_SHIFT | 1544u     ),
    VSUM4SHS_OPCODE= (4u  << OPCODE_SHIFT | 1608u     ),

    VAVGSB_OPCODE  = (4u  << OPCODE_SHIFT | 1282u     ),
    VAVGSW_OPCODE  = (4u  << OPCODE_SHIFT | 1410u     ),
    VAVGSH_OPCODE  = (4u  << OPCODE_SHIFT | 1346u     ),
    VAVGUB_OPCODE  = (4u  << OPCODE_SHIFT | 1026u     ),
    VAVGUW_OPCODE  = (4u  << OPCODE_SHIFT | 1154u     ),
    VAVGUH_OPCODE  = (4u  << OPCODE_SHIFT | 1090u     ),

    VMAXSB_OPCODE  = (4u  << OPCODE_SHIFT |  258u     ),
    VMAXSW_OPCODE  = (4u  << OPCODE_SHIFT |  386u     ),
    VMAXSH_OPCODE  = (4u  << OPCODE_SHIFT |  322u     ),
    VMAXUB_OPCODE  = (4u  << OPCODE_SHIFT |    2u     ),
    VMAXUW_OPCODE  = (4u  << OPCODE_SHIFT |  130u     ),
    VMAXUH_OPCODE  = (4u  << OPCODE_SHIFT |   66u     ),
    VMINSB_OPCODE  = (4u  << OPCODE_SHIFT |  770u     ),
    VMINSW_OPCODE  = (4u  << OPCODE_SHIFT |  898u     ),
    VMINSH_OPCODE  = (4u  << OPCODE_SHIFT |  834u     ),
    VMINUB_OPCODE  = (4u  << OPCODE_SHIFT |  514u     ),
    VMINUW_OPCODE  = (4u  << OPCODE_SHIFT |  642u     ),
    VMINUH_OPCODE  = (4u  << OPCODE_SHIFT |  578u     ),

    VCMPEQUB_OPCODE= (4u  << OPCODE_SHIFT |    6u     ),
    VCMPEQUH_OPCODE= (4u  << OPCODE_SHIFT |   70u     ),
    VCMPEQUW_OPCODE= (4u  << OPCODE_SHIFT |  134u     ),
    VCMPGTSH_OPCODE= (4u  << OPCODE_SHIFT |  838u     ),
    VCMPGTSB_OPCODE= (4u  << OPCODE_SHIFT |  774u     ),
    VCMPGTSW_OPCODE= (4u  << OPCODE_SHIFT |  902u     ),
    VCMPGTUB_OPCODE= (4u  << OPCODE_SHIFT |  518u     ),
    VCMPGTUH_OPCODE= (4u  << OPCODE_SHIFT |  582u     ),
    VCMPGTUW_OPCODE= (4u  << OPCODE_SHIFT |  646u     ),

    VAND_OPCODE    = (4u  << OPCODE_SHIFT | 1028u     ),
    VANDC_OPCODE   = (4u  << OPCODE_SHIFT | 1092u     ),
    VNOR_OPCODE    = (4u  << OPCODE_SHIFT | 1284u     ),
    VOR_OPCODE     = (4u  << OPCODE_SHIFT | 1156u     ),
    VXOR_OPCODE    = (4u  << OPCODE_SHIFT | 1220u     ),
    VRLD_OPCODE    = (4u  << OPCODE_SHIFT |  196u     ),
    VRLB_OPCODE    = (4u  << OPCODE_SHIFT |    4u     ),
    VRLW_OPCODE    = (4u  << OPCODE_SHIFT |  132u     ),
    VRLH_OPCODE    = (4u  << OPCODE_SHIFT |   68u     ),
    VSLB_OPCODE    = (4u  << OPCODE_SHIFT |  260u     ),
    VSKW_OPCODE    = (4u  << OPCODE_SHIFT |  388u     ),
    VSLH_OPCODE    = (4u  << OPCODE_SHIFT |  324u     ),
    VSRB_OPCODE    = (4u  << OPCODE_SHIFT |  516u     ),
    VSRW_OPCODE    = (4u  << OPCODE_SHIFT |  644u     ),
    VSRH_OPCODE    = (4u  << OPCODE_SHIFT |  580u     ),
    VSRAB_OPCODE   = (4u  << OPCODE_SHIFT |  772u     ),
    VSRAW_OPCODE   = (4u  << OPCODE_SHIFT |  900u     ),
    VSRAH_OPCODE   = (4u  << OPCODE_SHIFT |  836u     ),
    VPOPCNTW_OPCODE= (4u  << OPCODE_SHIFT | 1923u     ),

    // Vector Floating-Point
    // not implemented yet

    // Vector Status and Control
    MTVSCR_OPCODE  = (4u  << OPCODE_SHIFT | 1604u     ),
    MFVSCR_OPCODE  = (4u  << OPCODE_SHIFT | 1540u     ),

    // AES (introduced with Power 8)
    VCIPHER_OPCODE      = (4u  << OPCODE_SHIFT | 1288u),
    VCIPHERLAST_OPCODE  = (4u  << OPCODE_SHIFT | 1289u),
    VNCIPHER_OPCODE     = (4u  << OPCODE_SHIFT | 1352u),
    VNCIPHERLAST_OPCODE = (4u  << OPCODE_SHIFT | 1353u),
    VSBOX_OPCODE        = (4u  << OPCODE_SHIFT | 1480u),

    // SHA (introduced with Power 8)
    VSHASIGMAD_OPCODE   = (4u  << OPCODE_SHIFT | 1730u),
    VSHASIGMAW_OPCODE   = (4u  << OPCODE_SHIFT | 1666u),

    // Vector Binary Polynomial Multiplication (introduced with Power 8)
    VPMSUMB_OPCODE      = (4u  << OPCODE_SHIFT | 1032u),
    VPMSUMD_OPCODE      = (4u  << OPCODE_SHIFT | 1224u),
    VPMSUMH_OPCODE      = (4u  << OPCODE_SHIFT | 1096u),
    VPMSUMW_OPCODE      = (4u  << OPCODE_SHIFT | 1160u),

    // Vector Permute and Xor (introduced with Power 8)
    VPERMXOR_OPCODE     = (4u  << OPCODE_SHIFT |   45u),

    // Transactional Memory instructions (introduced with Power 8)
    TBEGIN_OPCODE    = (31u << OPCODE_SHIFT |  654u << 1),
    TEND_OPCODE      = (31u << OPCODE_SHIFT |  686u << 1),
    TABORT_OPCODE    = (31u << OPCODE_SHIFT |  910u << 1),
    TABORTWC_OPCODE  = (31u << OPCODE_SHIFT |  782u << 1),
    TABORTWCI_OPCODE = (31u << OPCODE_SHIFT |  846u << 1),
    TABORTDC_OPCODE  = (31u << OPCODE_SHIFT |  814u << 1),
    TABORTDCI_OPCODE = (31u << OPCODE_SHIFT |  878u << 1),
    TSR_OPCODE       = (31u << OPCODE_SHIFT |  750u << 1),
    TCHECK_OPCODE    = (31u << OPCODE_SHIFT |  718u << 1),

    // Icache and dcache related instructions
    DCBA_OPCODE    = (31u << OPCODE_SHIFT |  758u << 1),
    DCBZ_OPCODE    = (31u << OPCODE_SHIFT | 1014u << 1),
    DCBST_OPCODE   = (31u << OPCODE_SHIFT |   54u << 1),
    DCBF_OPCODE    = (31u << OPCODE_SHIFT |   86u << 1),

    DCBT_OPCODE    = (31u << OPCODE_SHIFT |  278u << 1),
    DCBTST_OPCODE  = (31u << OPCODE_SHIFT |  246u << 1),
    ICBI_OPCODE    = (31u << OPCODE_SHIFT |  982u << 1),

    // Instruction synchronization
    ISYNC_OPCODE   = (19u << OPCODE_SHIFT |  150u << 1),
    // Memory barriers
    SYNC_OPCODE    = (31u << OPCODE_SHIFT |  598u << 1),
    EIEIO_OPCODE   = (31u << OPCODE_SHIFT |  854u << 1),

    // Wait instructions for polling.
    WAIT_OPCODE    = (31u << OPCODE_SHIFT |   62u << 1),

    // Trap instructions
    TDI_OPCODE     = (2u  << OPCODE_SHIFT),
    TWI_OPCODE     = (3u  << OPCODE_SHIFT),
    TD_OPCODE      = (31u << OPCODE_SHIFT |   68u << 1),
    TW_OPCODE      = (31u << OPCODE_SHIFT |    4u << 1),

    // Atomics.
    LBARX_OPCODE   = (31u << OPCODE_SHIFT |   52u << 1),
    LHARX_OPCODE   = (31u << OPCODE_SHIFT |  116u << 1),
    LWARX_OPCODE   = (31u << OPCODE_SHIFT |   20u << 1),
    LDARX_OPCODE   = (31u << OPCODE_SHIFT |   84u << 1),
    LQARX_OPCODE   = (31u << OPCODE_SHIFT |  276u << 1),
    STBCX_OPCODE   = (31u << OPCODE_SHIFT |  694u << 1),
    STHCX_OPCODE   = (31u << OPCODE_SHIFT |  726u << 1),
    STWCX_OPCODE   = (31u << OPCODE_SHIFT |  150u << 1),
    STDCX_OPCODE   = (31u << OPCODE_SHIFT |  214u << 1),
    STQCX_OPCODE   = (31u << OPCODE_SHIFT |  182u << 1)

  };

  // Trap instructions TO bits
  enum trap_to_bits {
    // single bits
    traptoLessThanSigned      = 1 << 4, // 0, left end
    traptoGreaterThanSigned   = 1 << 3,
    traptoEqual               = 1 << 2,
    traptoLessThanUnsigned    = 1 << 1,
    traptoGreaterThanUnsigned = 1 << 0, // 4, right end

    // compound ones
    traptoUnconditional       = (traptoLessThanSigned |
                                 traptoGreaterThanSigned |
                                 traptoEqual |
                                 traptoLessThanUnsigned |
                                 traptoGreaterThanUnsigned)
  };

  // Branch hints BH field
  enum branch_hint_bh {
    // bclr cases:
    bhintbhBCLRisReturn            = 0,
    bhintbhBCLRisNotReturnButSame  = 1,
    bhintbhBCLRisNotPredictable    = 3,

    // bcctr cases:
    bhintbhBCCTRisNotReturnButSame = 0,
    bhintbhBCCTRisNotPredictable   = 3
  };

  // Branch prediction hints AT field
  enum branch_hint_at {
    bhintatNoHint     = 0,  // at=00
    bhintatIsNotTaken = 2,  // at=10
    bhintatIsTaken    = 3   // at=11
  };

  // Branch prediction hints
  enum branch_hint_concept {
    // Use the same encoding as branch_hint_at to simply code.
    bhintNoHint       = bhintatNoHint,
    bhintIsNotTaken   = bhintatIsNotTaken,
    bhintIsTaken      = bhintatIsTaken
  };

  // Used in BO field of branch instruction.
  enum branch_condition {
    bcondCRbiIs0      =  4, // bo=001at
    bcondCRbiIs1      = 12, // bo=011at
    bcondAlways       = 20  // bo=10100
  };

  // Branch condition with combined prediction hints.
  enum branch_condition_with_hint {
    bcondCRbiIs0_bhintNoHint     = bcondCRbiIs0 | bhintatNoHint,
    bcondCRbiIs0_bhintIsNotTaken = bcondCRbiIs0 | bhintatIsNotTaken,
    bcondCRbiIs0_bhintIsTaken    = bcondCRbiIs0 | bhintatIsTaken,
    bcondCRbiIs1_bhintNoHint     = bcondCRbiIs1 | bhintatNoHint,
    bcondCRbiIs1_bhintIsNotTaken = bcondCRbiIs1 | bhintatIsNotTaken,
    bcondCRbiIs1_bhintIsTaken    = bcondCRbiIs1 | bhintatIsTaken,
  };

  // Elemental Memory Barriers (>=Power 8)
  enum Elemental_Membar_mask_bits {
    StoreStore = 1 << 0,
    StoreLoad  = 1 << 1,
    LoadStore  = 1 << 2,
    LoadLoad   = 1 << 3
  };

  // Branch prediction hints.
  inline static int add_bhint_to_boint(const int bhint, const int boint) {
    switch (boint) {
      case bcondCRbiIs0:
      case bcondCRbiIs1:
        // branch_hint and branch_hint_at have same encodings
        assert(   (int)bhintNoHint     == (int)bhintatNoHint
               && (int)bhintIsNotTaken == (int)bhintatIsNotTaken
               && (int)bhintIsTaken    == (int)bhintatIsTaken,
               "wrong encodings");
        assert((bhint & 0x03) == bhint, "wrong encodings");
        return (boint & ~0x03) | bhint;
      case bcondAlways:
        // no branch_hint
        return boint;
      default:
        ShouldNotReachHere();
        return 0;
    }
  }

  // Extract bcond from boint.
  inline static int inv_boint_bcond(const int boint) {
    int r_bcond = boint & ~0x03;
    assert(r_bcond == bcondCRbiIs0 ||
           r_bcond == bcondCRbiIs1 ||
           r_bcond == bcondAlways,
           "bad branch condition");
    return r_bcond;
  }

  // Extract bhint from boint.
  inline static int inv_boint_bhint(const int boint) {
    int r_bhint = boint & 0x03;
    assert(r_bhint == bhintatNoHint ||
           r_bhint == bhintatIsNotTaken ||
           r_bhint == bhintatIsTaken,
           "bad branch hint");
    return r_bhint;
  }

  // Calculate opposite of given bcond.
  inline static int opposite_bcond(const int bcond) {
    switch (bcond) {
      case bcondCRbiIs0:
        return bcondCRbiIs1;
      case bcondCRbiIs1:
        return bcondCRbiIs0;
      default:
        ShouldNotReachHere();
        return 0;
    }
  }

  // Calculate opposite of given bhint.
  inline static int opposite_bhint(const int bhint) {
    switch (bhint) {
      case bhintatNoHint:
        return bhintatNoHint;
      case bhintatIsNotTaken:
        return bhintatIsTaken;
      case bhintatIsTaken:
        return bhintatIsNotTaken;
      default:
        ShouldNotReachHere();
        return 0;
    }
  }

  // RISCV branch instructions
  enum riscvops {
    b_op    = 18,
    bc_op   = 16,
    bcr_op  = 19
  };

  enum Condition {
    negative         = 0,
    less             = 0,
    positive         = 1,
    greater          = 1,
    zero             = 2,
    equal            = 2,
    summary_overflow = 3,
  };

 public:
  // Helper functions for groups of instructions

  enum Predict { pt = 1, pn = 0 }; // pt = predict taken

  //---<  calculate length of instruction  >---
  // With RISCV64 being a RISC architecture, this always is BytesPerInstWord
  // instruction must start at passed address
  static unsigned int instr_len(unsigned char *instr) { return BytesPerInstWord; }

  //---<  longest instructions  >---
  static unsigned int instr_maxlen() { return BytesPerInstWord; }

  // Test if x is within signed immediate range for nbits.
  static bool is_simm(int x, unsigned int nbits) {
    assert(0 < nbits && nbits < 32, "out of bounds");
    const int   min      = -(((int)1) << nbits-1);
    const int   maxplus1 =  (((int)1) << nbits-1);
    return min <= x && x < maxplus1;
  }

  static bool is_simm(jlong x, unsigned int nbits) {
    assert(0 < nbits && nbits < 64, "out of bounds");
    const jlong min      = -(((jlong)1) << nbits-1);
    const jlong maxplus1 =  (((jlong)1) << nbits-1);
    return min <= x && x < maxplus1;
  }

  // Test if x is within unsigned immediate range for nbits.
  static bool is_uimm(int x, unsigned int nbits) {
    assert(0 < nbits && nbits < 32, "out of bounds");
    const unsigned int maxplus1 = (((unsigned int)1) << nbits);
    return (unsigned int)x < maxplus1;
  }

  static bool is_uimm(jlong x, unsigned int nbits) {
    assert(0 < nbits && nbits < 64, "out of bounds");
    const julong maxplus1 = (((julong)1) << nbits);
    return (julong)x < maxplus1;
  }

 protected:
  // helpers

  // X is supposed to fit in a field "nbits" wide
  // and be sign-extended. Check the range.
  static void assert_signed_range(intptr_t x, int nbits) {
    assert(nbits == 32 || (-(1 << nbits-1) <= x && x < (1 << nbits-1)),
           "value out of range");
  }

  static void assert_signed_word_disp_range(intptr_t x, int nbits) {
    assert((x & 3) == 0, "not word aligned");
    assert_signed_range(x, nbits + 2);
  }

  static void assert_unsigned_const(int x, int nbits) {
    assert(juint(x) < juint(1 << nbits), "unsigned constant out of range");
  }

  static int fmask(juint hi_bit, juint lo_bit) {
    assert(hi_bit >= lo_bit && hi_bit < 32, "bad bits");
    return (1 << ( hi_bit-lo_bit + 1 )) - 1;
  }

  // inverse of u_field
  static int inv_u_field(int x, int hi_bit, int lo_bit) {
    juint r = juint(x) >> lo_bit;
    r &= fmask(hi_bit, lo_bit);
    return int(r);
  }

  // signed version: extract from field and sign-extend
  static int inv_s_field_riscv(int x, int hi_bit, int lo_bit) {
    x = x << (31-hi_bit);
    x = x >> (31-hi_bit+lo_bit);
    return x;
  }

  static int u_field(int x, int hi_bit, int lo_bit) {
    assert((x & ~fmask(hi_bit, lo_bit)) == 0, "value out of range");
    int r = x << lo_bit;
    assert(inv_u_field(r, hi_bit, lo_bit) == x, "just checking");
    return r;
  }

  // Same as u_field for signed values
  static int s_field(int x, int hi_bit, int lo_bit) {
    int nbits = hi_bit - lo_bit + 1;
    assert(nbits == 32 || (-(1 << nbits-1) <= x && x < (1 << nbits-1)),
      "value out of range");
    x &= fmask(hi_bit, lo_bit);
    int r = x << lo_bit;
    return r;
  }

  static int get_opcode(int x) { return x & 0x7f; }

  // Determine target address from li, bd field of branch instruction.
  static intptr_t inv_li_field(int x) {
    intptr_t r = inv_s_field_riscv(x, 25, 2);
    r = (r << 2);
    return r;
  }
  static intptr_t inv_bd_field(int x, intptr_t pos) {
    intptr_t r = inv_s_field_riscv(x, 15, 2);
    r = (r << 2) + pos;
    return r;
  }

  #define inv_opp_u_field(x, hi_bit, lo_bit) inv_u_field(x, 31-(lo_bit), 31-(hi_bit))
  #define inv_opp_s_field(x, hi_bit, lo_bit) inv_s_field_riscv(x, 31-(lo_bit), 31-(hi_bit))
  // Extract instruction fields from instruction words.
 public:
  static int inv_ra_field(int x)  { return inv_opp_u_field(x, 15, 11); }
  static int inv_rb_field(int x)  { return inv_opp_u_field(x, 20, 16); }
  static int inv_rt_field(int x)  { return inv_opp_u_field(x, 10,  6); }
  static int inv_rta_field(int x) { return inv_opp_u_field(x, 15, 11); }
  static int inv_rs_field(int x)  { return inv_opp_u_field(x, 10,  6); }
  // Ds uses opp_s_field(x, 31, 16), but lowest 2 bits must be 0.
  // Inv_ds_field uses range (x, 29, 16) but shifts by 2 to ensure that lowest bits are 0.
  static int inv_ds_field(int x)  { return inv_opp_s_field(x, 29, 16) << 2; }
  static int inv_d1_field(int x)  { return inv_opp_s_field(x, 31, 16); }
  static int inv_si_field(int x)  { return inv_opp_s_field(x, 31, 16); }
  static int inv_to_field(int x)  { return inv_opp_u_field(x, 10, 6);  }
  static int inv_lk_field(int x)  { return inv_opp_u_field(x, 31, 31); }
  static int inv_bo_field(int x)  { return inv_opp_u_field(x, 10,  6); }
  static int inv_bi_field(int x)  { return inv_opp_u_field(x, 15, 11); }

  #define opp_u_field(x, hi_bit, lo_bit) u_field(x, 31-(lo_bit), 31-(hi_bit))
  #define opp_s_field(x, hi_bit, lo_bit) s_field(x, 31-(lo_bit), 31-(hi_bit))

  // instruction fields
  static int aa(       int         x)  { return  opp_u_field(x,             30, 30); }
  static int ba(       int         x)  { return  opp_u_field(x,             15, 11); }
  static int bb(       int         x)  { return  opp_u_field(x,             20, 16); }
  static int bc(       int         x)  { return  opp_u_field(x,             25, 21); }
  static int bd(       int         x)  { return  opp_s_field(x,             29, 16); }
  static int bf( ConditionRegister cr) { return  bf(cr->encoding()); }
  static int bf(       int         x)  { return  opp_u_field(x,              8,  6); }
  static int bfa(ConditionRegister cr) { return  bfa(cr->encoding()); }
  static int bfa(      int         x)  { return  opp_u_field(x,             13, 11); }
  static int bh(       int         x)  { return  opp_u_field(x,             20, 19); }
  static int bi(       int         x)  { return  opp_u_field(x,             15, 11); }
  static int bi0(ConditionRegister cr, Condition c) { return (cr->encoding() << 2) | c; }
  static int bo(       int         x)  { return  opp_u_field(x,             10,  6); }
  static int bt(       int         x)  { return  opp_u_field(x,             10,  6); }
  static int d1(       int         x)  { return  opp_s_field(x,             31, 16); }
  static int ds(       int         x)  { assert((x & 0x3) == 0, "unaligned offset"); return opp_s_field(x, 31, 16); }
  static int eh(       int         x)  { return  opp_u_field(x,             31, 31); }
  static int flm(      int         x)  { return  opp_u_field(x,             14,  7); }
  static int fra(    FloatRegister r)  { return  fra(r->encoding());}
  static int frb(    FloatRegister r)  { return  frb(r->encoding());}
  static int frc(    FloatRegister r)  { return  frc(r->encoding());}
  static int frs(    FloatRegister r)  { return  frs(r->encoding());}
  static int frt(    FloatRegister r)  { return  frt(r->encoding());}
  static int fra(      int         x)  { return  opp_u_field(x,             15, 11); }
  static int frb(      int         x)  { return  opp_u_field(x,             20, 16); }
  static int frc(      int         x)  { return  opp_u_field(x,             25, 21); }
  static int frs(      int         x)  { return  opp_u_field(x,             10,  6); }
  static int frt(      int         x)  { return  opp_u_field(x,             10,  6); }
  static int fxm(      int         x)  { return  opp_u_field(x,             19, 12); }
  static int l10(      int         x)  { assert(x == 0 || x == 1,  "must be 0 or 1"); return opp_u_field(x, 10, 10); }
  static int l14(      int         x)  { return  opp_u_field(x,             15, 14); }
  static int l15(      int         x)  { return  opp_u_field(x,             15, 15); }
  static int l910(     int         x)  { return  opp_u_field(x,             10,  9); }
  static int e1215(    int         x)  { return  opp_u_field(x,             15, 12); }
  static int lev(      int         x)  { return  opp_u_field(x,             26, 20); }
  static int li_PPC(       int         x)  { return  opp_s_field(x,             29,  6); }
  static int lk(       int         x)  { return  opp_u_field(x,             31, 31); }
  static int mb2125(   int         x)  { return  opp_u_field(x,             25, 21); }
  static int me2630(   int         x)  { return  opp_u_field(x,             30, 26); }
  static int mb2126(   int         x)  { return  opp_u_field(((x & 0x1f) << 1) | ((x & 0x20) >> 5), 26, 21); }
  static int me2126(   int         x)  { return  mb2126(x); }
  static int nb(       int         x)  { return  opp_u_field(x,             20, 16); }
  //static int opcd(   int         x)  { return  opp_u_field(x,              5,  0); } // is contained in our opcodes
  static int oe(       int         x)  { return  opp_u_field(x,             21, 21); }
  static int ra(       Register    r)  { return  ra(r->encoding()); }
  static int ra(       int         x)  { return  opp_u_field(x,             15, 11); }
  static int rb(       Register    r)  { return  rb(r->encoding()); }
  static int rb(       int         x)  { return  opp_u_field(x,             20, 16); }
  static int rc(       int         x)  { return  opp_u_field(x,             31, 31); }
  static int rs(       Register    r)  { return  rs(r->encoding()); }
  static int rs(       int         x)  { return  opp_u_field(x,             10,  6); }
  // we don't want to use R0 in memory accesses, because it has value `0' then
  static int ra0mem(   Register    r)  { assert(r != R0, "cannot use register R0 in memory access"); return ra(r); }
  static int ra0mem(   int         x)  { assert(x != 0,  "cannot use register 0 in memory access");  return ra(x); }

  // register r is target
  static int rt(       Register    r)  { return rs(r); }
  static int rt(       int         x)  { return rs(x); }
  static int rta(      Register    r)  { return ra(r); }
  static int rta0mem(  Register    r)  { rta(r); return ra0mem(r); }

  static int sh1620(   int         x)  { return  opp_u_field(x,             20, 16); }
  static int sh30(     int         x)  { return  opp_u_field(x,             30, 30); }
  static int sh162030( int         x)  { return  sh1620(x & 0x1f) | sh30((x & 0x20) >> 5); }
  static int si(       int         x)  { return  opp_s_field(x,             31, 16); }
  static int spr(      int         x)  { return  opp_u_field(x,             20, 11); }
  static int sr(       int         x)  { return  opp_u_field(x,             15, 12); }
  static int tbr(      int         x)  { return  opp_u_field(x,             20, 11); }
  static int th(       int         x)  { return  opp_u_field(x,             10,  7); }
  static int thct(     int         x)  { assert((x&8) == 0, "must be valid cache specification");  return th(x); }
  static int thds(     int         x)  { assert((x&8) == 8, "must be valid stream specification"); return th(x); }
  static int to(       int         x)  { return  opp_u_field(x,             10,  6); }
  static int u(        int         x)  { return  opp_u_field(x,             19, 16); }
  static int ui(       int         x)  { return  opp_u_field(x,             31, 16); }

  // Support vector instructions for >= Power6.
  static int vra(      int         x)  { return  opp_u_field(x,             15, 11); }
  static int vrb(      int         x)  { return  opp_u_field(x,             20, 16); }
  static int vrc(      int         x)  { return  opp_u_field(x,             25, 21); }
  static int vrs(      int         x)  { return  opp_u_field(x,             10,  6); }
  static int vrt(      int         x)  { return  opp_u_field(x,             10,  6); }

  static int vra(   VectorRegister r)  { return  vra(r->encoding());}
  static int vrb(   VectorRegister r)  { return  vrb(r->encoding());}
  static int vrc(   VectorRegister r)  { return  vrc(r->encoding());}
  static int vrs(   VectorRegister r)  { return  vrs(r->encoding());}
  static int vrt(   VectorRegister r)  { return  vrt(r->encoding());}

  // Only used on SHA sigma instructions (VX-form)
  static int vst(      int         x)  { return  opp_u_field(x,             16, 16); }
  static int vsix(     int         x)  { return  opp_u_field(x,             20, 17); }

  // Support Vector-Scalar (VSX) instructions.
  static int vsra(      int         x)  { return  opp_u_field(x & 0x1F,     15, 11) | opp_u_field((x & 0x20) >> 5, 29, 29); }
  static int vsrb_PPC(      int         x)  { return  opp_u_field(x & 0x1F,     20, 16) | opp_u_field((x & 0x20) >> 5, 30, 30); }
  static int vsrs(      int         x)  { return  opp_u_field(x & 0x1F,     10,  6) | opp_u_field((x & 0x20) >> 5, 31, 31); }
  static int vsrt(      int         x)  { return  vsrs(x); }
  static int vsdm(      int         x)  { return  opp_u_field(x,            23, 22); }

  static int vsra(   VectorSRegister r)  { return  vsra(r->encoding());}
  static int vsrb_PPC(   VectorSRegister r)  { return  vsrb_PPC(r->encoding());}
  static int vsrs(   VectorSRegister r)  { return  vsrs(r->encoding());}
  static int vsrt(   VectorSRegister r)  { return  vsrt(r->encoding());}

  static int vsplt_uim( int        x)  { return  opp_u_field(x,             15, 12); } // for vsplt* instructions
  static int vsplti_sim(int        x)  { return  opp_u_field(x,             15, 11); } // for vsplti* instructions
  static int vsldoi_shb(int        x)  { return  opp_u_field(x,             25, 22); } // for vsldoi instruction
  static int vcmp_rc(   int        x)  { return  opp_u_field(x,             21, 21); } // for vcmp* instructions
  static int xxsplt_uim(int        x)  { return  opp_u_field(x,             15, 14); } // for xxsplt* instructions

  //static int xo1(     int        x)  { return  opp_u_field(x,             29, 21); }// is contained in our opcodes
  //static int xo2(     int        x)  { return  opp_u_field(x,             30, 21); }// is contained in our opcodes
  //static int xo3(     int        x)  { return  opp_u_field(x,             30, 22); }// is contained in our opcodes
  //static int xo4(     int        x)  { return  opp_u_field(x,             30, 26); }// is contained in our opcodes
  //static int xo5(     int        x)  { return  opp_u_field(x,             29, 27); }// is contained in our opcodes
  //static int xo6(     int        x)  { return  opp_u_field(x,             30, 27); }// is contained in our opcodes
  //static int xo7(     int        x)  { return  opp_u_field(x,             31, 30); }// is contained in our opcodes

 protected:
  // Compute relative address for branch.
  static intptr_t disp(intptr_t x, intptr_t off) {
    int xx = x - off;
    return xx;
  }

 public:
  // signed immediate, in low bits, nbits long
  static int simm(int x, int nbits) {
    assert_signed_range(x, nbits);
    return x & ((1 << nbits) - 1);
  }

  // unsigned immediate, in low bits, nbits long
  static int uimm(int x, int nbits) {
    assert_unsigned_const(x, nbits);
    return x & ((1 << nbits) - 1);
  }

  static void set_imm(int* instr, short s) {
    // imm is always in the lower 16 bits of the instruction,
    // so this is endian-neutral. Same for the get_imm below.
    uint32_t w = *(uint32_t *)instr;
    *instr = (int)((w & ~0x0000FFFF) | (s & 0x0000FFFF));
  }

  static int get_imm(address a, int instruction_number) {
    return (short)((int *)a)[instruction_number];
  }

  static inline int hi16_signed(  int x) { return (int)(int16_t)(x >> 16); }
  static inline int lo16_unsigned(int x) { return x & 0xffff; }

 protected:

  // Extract the top 32 bits in a 64 bit word.
  static int32_t hi32(int64_t x) {
    int32_t r = int32_t((uint64_t)x >> 32);
    return r;
  }

 public:

  static inline unsigned int align_addr(unsigned int addr, unsigned int a) {
    return ((addr + (a - 1)) & ~(a - 1));
  }

  static inline bool is_aligned(unsigned int addr, unsigned int a) {
    return (0 == addr % a);
  }

  void flush() {
    AbstractAssembler::flush();
  }

  inline void emit_int32(int);  // shadows AbstractAssembler::emit_int32
  inline void emit_data(int);
  inline void emit_data(int, RelocationHolder const&);
  inline void emit_data(int, relocInfo::relocType rtype);

  // Emit an address.
  inline address emit_addr(const address addr = NULL);

  /////////////////////////////////////////////////////////////////////////////////////
  // PPC instructions
  /////////////////////////////////////////////////////////////////////////////////////

  // Memory instructions use r0 as hard coded 0, e.g. to simulate loading
  // immediates. The normal instruction encoders enforce that r0 is not
  // passed to them. Use either extended mnemonics encoders or the special ra0
  // versions.

  // PPC 1, section 3.3.8, Fixed-Point Arithmetic Instructions
  inline void addi_PPC( Register d, Register a, int si16);
  inline void addis_PPC(Register d, Register a, int si16);
 private:
  inline void addi_r0ok_PPC( Register d, Register a, int si16);
  inline void addis_r0ok_PPC(Register d, Register a, int si16);
 public:
  inline void addic__PPC( Register d, Register a, int si16);
  inline void subfic_PPC( Register d, Register a, int si16);
  inline void add_PPC(    Register d, Register a, Register b);
  inline void add__PPC(   Register d, Register a, Register b);
  inline void subf_PPC(   Register d, Register a, Register b);  // d = b - a    "Sub_from", as in riscv spec.
  inline void sub_PPC(    Register d, Register a, Register b);  // d = a - b    Swap operands of subf for readability.
  inline void subf__PPC(  Register d, Register a, Register b);
  inline void addc_PPC(   Register d, Register a, Register b);
  inline void addc__PPC(  Register d, Register a, Register b);
  inline void subfc_PPC(  Register d, Register a, Register b);
  inline void subfc__PPC( Register d, Register a, Register b);
  inline void adde_PPC(   Register d, Register a, Register b);
  inline void adde__PPC(  Register d, Register a, Register b);
  inline void subfe_PPC(  Register d, Register a, Register b);
  inline void subfe__PPC( Register d, Register a, Register b);
  inline void addme_PPC(  Register d, Register a);
  inline void addme__PPC( Register d, Register a);
  inline void subfme_PPC( Register d, Register a);
  inline void subfme__PPC(Register d, Register a);
  inline void addze_PPC(  Register d, Register a);
  inline void addze__PPC( Register d, Register a);
  inline void subfze_PPC( Register d, Register a);
  inline void subfze__PPC(Register d, Register a);
  inline void neg_PPC(    Register d, Register a);
  inline void neg__PPC(   Register d, Register a);
  inline void mulli_PPC(  Register d, Register a, int si16);
  inline void mulld_PPC(  Register d, Register a, Register b);
  inline void mulld__PPC( Register d, Register a, Register b);
  inline void mullw_PPC(  Register d, Register a, Register b);
  inline void mullw__PPC( Register d, Register a, Register b);
  inline void mulhw_PPC(  Register d, Register a, Register b);
  inline void mulhw__PPC( Register d, Register a, Register b);
  inline void mulhwu_PPC( Register d, Register a, Register b);
  inline void mulhwu__PPC(Register d, Register a, Register b);
  inline void mulhd_PPC(  Register d, Register a, Register b);
  inline void mulhd__PPC( Register d, Register a, Register b);
  inline void mulhdu_PPC( Register d, Register a, Register b);
  inline void mulhdu__PPC(Register d, Register a, Register b);
  inline void divd_PPC(   Register d, Register a, Register b);
  inline void divd__PPC(  Register d, Register a, Register b);
  inline void divw_PPC(   Register d, Register a, Register b);
  inline void divw__PPC(  Register d, Register a, Register b);

  // Fixed-Point Arithmetic Instructions with Overflow detection
  inline void addo_PPC(    Register d, Register a, Register b);
  inline void addo__PPC(   Register d, Register a, Register b);
  inline void subfo_PPC(   Register d, Register a, Register b);
  inline void subfo__PPC(  Register d, Register a, Register b);
  inline void addco_PPC(   Register d, Register a, Register b);
  inline void addco__PPC(  Register d, Register a, Register b);
  inline void subfco_PPC(  Register d, Register a, Register b);
  inline void subfco__PPC( Register d, Register a, Register b);
  inline void addeo_PPC(   Register d, Register a, Register b);
  inline void addeo__PPC(  Register d, Register a, Register b);
  inline void subfeo_PPC(  Register d, Register a, Register b);
  inline void subfeo__PPC( Register d, Register a, Register b);
  inline void addmeo_PPC(  Register d, Register a);
  inline void addmeo__PPC( Register d, Register a);
  inline void subfmeo_PPC( Register d, Register a);
  inline void subfmeo__PPC(Register d, Register a);
  inline void addzeo_PPC(  Register d, Register a);
  inline void addzeo__PPC( Register d, Register a);
  inline void subfzeo_PPC( Register d, Register a);
  inline void subfzeo__PPC(Register d, Register a);
  inline void nego_PPC(    Register d, Register a);
  inline void nego__PPC(   Register d, Register a);
  inline void mulldo_PPC(  Register d, Register a, Register b);
  inline void mulldo__PPC( Register d, Register a, Register b);
  inline void mullwo_PPC(  Register d, Register a, Register b);
  inline void mullwo__PPC( Register d, Register a, Register b);
  inline void divdo_PPC(   Register d, Register a, Register b);
  inline void divdo__PPC(  Register d, Register a, Register b);
  inline void divwo_PPC(   Register d, Register a, Register b);
  inline void divwo__PPC(  Register d, Register a, Register b);

  // extended mnemonics
  inline void li_PPC(   Register d, int si16);
  inline void lis_PPC(  Register d, int si16);
  inline void addir_PPC(Register d, int si16, Register a);
  inline void subi_PPC( Register d, Register a, int si16);

  static bool is_addi(int x) {
     return ADDI_OPCODE == (x & ADDI_OPCODE_MASK);
  }
  static bool is_addis(int x) {
     return ADDIS_OPCODE == (x & ADDIS_OPCODE_MASK);
  }
  static bool is_bxx(int x) {
     return BXX_OPCODE == (x & BXX_OPCODE_MASK);
  }
  static bool is_b(int x) {
     return BXX_OPCODE == (x & BXX_OPCODE_MASK) && inv_lk_field(x) == 0;
  }
  static bool is_bl(int x) {
     return BXX_OPCODE == (x & BXX_OPCODE_MASK) && inv_lk_field(x) == 1;
  }
  static bool is_bcxx(int x) {
     return BCXX_OPCODE == (x & BCXX_OPCODE_MASK);
  }
  static bool is_bxx_or_bcxx(int x) {
     return is_bxx(x) || is_bcxx(x);
  }
  static bool is_bctrl(int x) {
     return x == 0x4e800421;
  }
  static bool is_bctr(int x) {
     return x == 0x4e800420;
  }
  static bool is_bclr(int x) {
     return BCLR_OPCODE == (x & XL_FORM_OPCODE_MASK);
  }
  static bool is_li(int x) {
     return is_addi(x) && inv_ra_field(x)==0;
  }
  static bool is_lis(int x) {
     return is_addis(x) && inv_ra_field(x)==0;
  }
  static bool is_mtctr(int x) {
     return MTCTR_OPCODE == (x & MTCTR_OPCODE_MASK);
  }
  static bool is_ld(int x) {
     return LD_OPCODE == (x & LD_OPCODE_MASK);
  }
  static bool is_std(int x) {
     return STD_OPCODE == (x & STD_OPCODE_MASK);
  }
  static bool is_stdu(int x) {
     return STDU_OPCODE == (x & STDU_OPCODE_MASK);
  }
  static bool is_stdx(int x) {
     return STDX_OPCODE == (x & STDX_OPCODE_MASK);
  }
  static bool is_stdux(int x) {
     return STDUX_OPCODE == (x & STDUX_OPCODE_MASK);
  }
  static bool is_stwx(int x) {
     return STWX_OPCODE == (x & STWX_OPCODE_MASK);
  }
  static bool is_stwux(int x) {
     return STWUX_OPCODE == (x & STWUX_OPCODE_MASK);
  }
  static bool is_stw(int x) {
     return STW_OPCODE == (x & STW_OPCODE_MASK);
  }
  static bool is_stwu(int x) {
     return STWU_OPCODE == (x & STWU_OPCODE_MASK);
  }
  static bool is_ori(int x) {
     return ORI_OPCODE == (x & ORI_OPCODE_MASK);
  };
  static bool is_oris(int x) {
     return ORIS_OPCODE == (x & ORIS_OPCODE_MASK);
  };
  static bool is_rldicr(int x) {
     return (RLDICR_OPCODE == (x & RLDICR_OPCODE_MASK));
  };
  static bool is_nop(int x) {
    return x == 0x60000000;
  }
  // endgroup opcode for Power6
  static bool is_endgroup(int x) {
    return is_ori(x) && inv_ra_field(x) == 1 && inv_rs_field(x) == 1 && inv_d1_field(x) == 0;
  }


 private:
  // RISCV 1, section 3.3.9, Fixed-Point Compare Instructions
  inline void cmpi_PPC( ConditionRegister bf, int l, Register a, int si16);
  inline void cmp_PPC(  ConditionRegister bf, int l, Register a, Register b);
  inline void cmpli_PPC(ConditionRegister bf, int l, Register a, int ui16);
  inline void cmpl_PPC( ConditionRegister bf, int l, Register a, Register b);

 public:
  // extended mnemonics of Compare Instructions
  inline void cmpwi_PPC( ConditionRegister crx, Register a, int si16);
  inline void cmpdi_PPC( ConditionRegister crx, Register a, int si16);
  inline void cmpw_PPC(  ConditionRegister crx, Register a, Register b);
  inline void cmpd_PPC(  ConditionRegister crx, Register a, Register b);
  inline void cmplwi_PPC(ConditionRegister crx, Register a, int ui16);
  inline void cmpldi_PPC(ConditionRegister crx, Register a, int ui16);
  inline void cmplw_PPC( ConditionRegister crx, Register a, Register b);
  inline void cmpld_PPC( ConditionRegister crx, Register a, Register b);

  // >= Power9
  inline void cmprb_PPC( ConditionRegister bf, int l, Register a, Register b);
  inline void cmpeqb_PPC(ConditionRegister bf, Register a, Register b);

  inline void isel_PPC(   Register d, Register a, Register b, int bc);
  // Convenient version which takes: Condition register, Condition code and invert flag. Omit b to keep old value.
  inline void isel_PPC(   Register d, ConditionRegister cr, Condition cc, bool inv, Register a, Register b = noreg);
  // Set d = 0 if (cr.cc) equals 1, otherwise b.
  inline void isel_0_PPC( Register d, ConditionRegister cr, Condition cc, Register b = noreg);

  // RISCV 1, section 3.3.11, Fixed-Point Logical Instructions
         void andi(   Register a, Register s, long ui16);   // optimized version
  inline void andi__PPC(  Register a, Register s, int ui16);
  inline void andis__PPC( Register a, Register s, int ui16);
  inline void ori_PPC(    Register a, Register s, int ui16);
  inline void oris_PPC(   Register a, Register s, int ui16);
  inline void xori_PPC(   Register a, Register s, int ui16);
  inline void xoris_PPC(  Register a, Register s, int ui16);
  inline void andr_PPC(   Register a, Register s, Register b);  // suffixed by 'r' as 'and' is C++ keyword
  inline void and__PPC(   Register a, Register s, Register b);
  // Turn or0(rx,rx,rx) into a nop and avoid that we accidently emit a
  // SMT-priority change instruction (see SMT instructions below).
  inline void or_unchecked_PPC(Register a, Register s, Register b);
  inline void orr_PPC(    Register a, Register s, Register b);  // suffixed by 'r' as 'or' is C++ keyword
  inline void or__PPC(    Register a, Register s, Register b);
  inline void xorr_PPC(   Register a, Register s, Register b);  // suffixed by 'r' as 'xor' is C++ keyword
  inline void xor__PPC(   Register a, Register s, Register b);
  inline void nand_PPC(   Register a, Register s, Register b);
  inline void nand__PPC(  Register a, Register s, Register b);
  inline void nor_PPC(    Register a, Register s, Register b);
  inline void nor__PPC(   Register a, Register s, Register b);
  inline void andc_PPC(   Register a, Register s, Register b);
  inline void andc__PPC(  Register a, Register s, Register b);
  inline void orc_PPC(    Register a, Register s, Register b);
  inline void orc__PPC(   Register a, Register s, Register b);
  inline void extsb_PPC(  Register a, Register s);
  inline void extsb__PPC( Register a, Register s);
  inline void extsh_PPC(  Register a, Register s);
  inline void extsh__PPC( Register a, Register s);
  inline void extsw_PPC(  Register a, Register s);
  inline void extsw__PPC( Register a, Register s);

  // extended mnemonics
  inline void nop_PPC();
  // NOP for FP and BR units (different versions to allow them to be in one group)
  inline void fpnop0_PPC();
  inline void fpnop1_PPC();
  inline void brnop0_PPC();
  inline void brnop1_PPC();
  inline void brnop2_PPC();

  inline void mr_PPC(      Register d, Register s);
  inline void ori_opt_PPC( Register d, int ui16);
  inline void oris_opt_PPC(Register d, int ui16);

  // endgroup opcode for Power6
  inline void endgroup_PPC();

  // count instructions
  inline void cntlzw_PPC(  Register a, Register s);
  inline void cntlzw__PPC( Register a, Register s);
  inline void cntlzd_PPC(  Register a, Register s);
  inline void cntlzd__PPC( Register a, Register s);
  inline void cnttzw_PPC(  Register a, Register s);
  inline void cnttzw__PPC( Register a, Register s);
  inline void cnttzd_PPC(  Register a, Register s);
  inline void cnttzd__PPC( Register a, Register s);

  // RISCV 1, section 3.3.12, Fixed-Point Rotate and Shift Instructions
  inline void sld_PPC(     Register a, Register s, Register b);
  inline void sld__PPC(    Register a, Register s, Register b);
  inline void slw_PPC(     Register a, Register s, Register b);
  inline void slw__PPC(    Register a, Register s, Register b);
  inline void srd_PPC(     Register a, Register s, Register b);
  inline void srd__PPC(    Register a, Register s, Register b);
  inline void srw_PPC(     Register a, Register s, Register b);
  inline void srw__PPC(    Register a, Register s, Register b);
  inline void srad_PPC(    Register a, Register s, Register b);
  inline void srad__PPC(   Register a, Register s, Register b);
  inline void sraw_PPC(    Register a, Register s, Register b);
  inline void sraw__PPC(   Register a, Register s, Register b);
  inline void sradi_PPC(   Register a, Register s, int sh6);
  inline void sradi__PPC(  Register a, Register s, int sh6);
  inline void srawi_PPC(   Register a, Register s, int sh5);
  inline void srawi__PPC(  Register a, Register s, int sh5);

  // extended mnemonics for Shift Instructions
  inline void sldi_PPC(    Register a, Register s, int sh6);
  inline void sldi__PPC(   Register a, Register s, int sh6);
  inline void slwi_PPC(    Register a, Register s, int sh5);
  inline void slwi__PPC(   Register a, Register s, int sh5);
  inline void srdi_PPC(    Register a, Register s, int sh6);
  inline void srdi__PPC(   Register a, Register s, int sh6);
  inline void srwi_PPC(    Register a, Register s, int sh5);
  inline void srwi__PPC(   Register a, Register s, int sh5);

  inline void clrrdi_PPC(  Register a, Register s, int ui6);
  inline void clrrdi__PPC( Register a, Register s, int ui6);
  inline void clrldi_PPC(  Register a, Register s, int ui6);
  inline void clrldi__PPC( Register a, Register s, int ui6);
  inline void clrlsldi_PPC(Register a, Register s, int clrl6, int shl6);
  inline void clrlsldi__PPC(Register a, Register s, int clrl6, int shl6);
  inline void extrdi_PPC(  Register a, Register s, int n, int b);
  // testbit with condition register
  inline void testbitdi_PPC(ConditionRegister cr, Register a, Register s, int ui6);

  // rotate instructions
  inline void rotldi_PPC(  Register a, Register s, int n);
  inline void rotrdi_PPC(  Register a, Register s, int n);
  inline void rotlwi_PPC(  Register a, Register s, int n);
  inline void rotrwi_PPC(  Register a, Register s, int n);

  // Rotate Instructions
  inline void rldic_PPC(   Register a, Register s, int sh6, int mb6);
  inline void rldic__PPC(  Register a, Register s, int sh6, int mb6);
  inline void rldicr_PPC(  Register a, Register s, int sh6, int mb6);
  inline void rldicr__PPC( Register a, Register s, int sh6, int mb6);
  inline void rldicl_PPC(  Register a, Register s, int sh6, int mb6);
  inline void rldicl__PPC( Register a, Register s, int sh6, int mb6);
  inline void rlwinm_PPC(  Register a, Register s, int sh5, int mb5, int me5);
  inline void rlwinm__PPC( Register a, Register s, int sh5, int mb5, int me5);
  inline void rldimi_PPC(  Register a, Register s, int sh6, int mb6);
  inline void rldimi__PPC( Register a, Register s, int sh6, int mb6);
  inline void rlwimi_PPC(  Register a, Register s, int sh5, int mb5, int me5);
  inline void insrdi_PPC(  Register a, Register s, int n,   int b);
  inline void insrwi_PPC(  Register a, Register s, int n,   int b);

  // RISCV 1, section 3.3.2 Fixed-Point Load Instructions
  // 4 bytes
  inline void lwzx_PPC( Register d, Register s1, Register s2);
  inline void lwz_PPC(  Register d, int si16,    Register s1);
  inline void lwzu_PPC( Register d, int si16,    Register s1);

  // 4 bytes
  inline void lwax_PPC( Register d, Register s1, Register s2);
  inline void lwa_PPC(  Register d, int si16,    Register s1);

  // 4 bytes reversed
  inline void lwbrx_PPC( Register d, Register s1, Register s2);

  // 2 bytes
  inline void lhzx_PPC( Register d, Register s1, Register s2);
  inline void lhz_PPC(  Register d, int si16,    Register s1);
  inline void lhzu_PPC( Register d, int si16,    Register s1);

  // 2 bytes reversed
  inline void lhbrx_PPC( Register d, Register s1, Register s2);

  // 2 bytes
  inline void lhax_PPC( Register d, Register s1, Register s2);
  inline void lha_PPC(  Register d, int si16,    Register s1);
  inline void lhau_PPC( Register d, int si16,    Register s1);

  // 1 byte
  inline void lbzx_PPC( Register d, Register s1, Register s2);
  inline void lbz_PPC(  Register d, int si16,    Register s1);
  inline void lbzu_PPC( Register d, int si16,    Register s1);

  // 8 bytes
  inline void ldx_PPC(  Register d, Register s1, Register s2);
  inline void ld_PPC(   Register d, int si16,    Register s1);
  inline void ldu_PPC(  Register d, int si16,    Register s1);

  // 8 bytes reversed
  inline void ldbrx_PPC( Register d, Register s1, Register s2);

  // For convenience. Load pointer into d from b+s1.
  inline void ld_ptr_PPC(Register d, int b, Register s1);
  DEBUG_ONLY(inline void ld_ptr_PPC(Register d, ByteSize b, Register s1);)

  //  RISCV 1, section 3.3.3 Fixed-Point Store Instructions
  inline void stwx_PPC( Register d, Register s1, Register s2);
  inline void stw_PPC(  Register d, int si16,    Register s1);
  inline void stwu_PPC( Register d, int si16,    Register s1);
  inline void stwbrx_PPC( Register d, Register s1, Register s2);

  inline void sthx_PPC( Register d, Register s1, Register s2);
  inline void sth_PPC(  Register d, int si16,    Register s1);
  inline void sthu_PPC( Register d, int si16,    Register s1);
  inline void sthbrx_PPC( Register d, Register s1, Register s2);

  inline void stbx_PPC( Register d, Register s1, Register s2);
  inline void stb_PPC(  Register d, int si16,    Register s1);
  inline void stbu_PPC( Register d, int si16,    Register s1);

  inline void stdx_PPC( Register d, Register s1, Register s2);
  inline void std_PPC(  Register d, int si16,    Register s1);
  inline void stdu_PPC( Register d, int si16,    Register s1);
  inline void stdux_PPC(Register s, Register a,  Register b);
  inline void stdbrx_PPC( Register d, Register s1, Register s2);

  inline void st_ptr_PPC(Register d, int si16,    Register s1);
  DEBUG_ONLY(inline void st_ptr_PPC(Register d, ByteSize b, Register s1);)

  // RISCV 1, section 3.3.13 Move To/From System Register Instructions
  inline void mtlr_PPC( Register s1);
  inline void mflr_PPC( Register d);
  inline void mtctr_PPC(Register s1);
  inline void mfctr_PPC(Register d);
  inline void mtcrf_PPC(int fxm, Register s);
  inline void mfcr_PPC( Register d);
  inline void mcrf_PPC( ConditionRegister crd, ConditionRegister cra);
  inline void mtcr_PPC( Register s);
  // >= Power9
  inline void setb_PPC( Register d, ConditionRegister cra);

  // Special purpose registers
  // Exception Register
  inline void mtxer_PPC(Register s1);
  inline void mfxer_PPC(Register d);
  // Vector Register Save Register
  inline void mtvrsave_PPC(Register s1);
  inline void mfvrsave_PPC(Register d);
  // Timebase
  inline void mftb_PPC(Register d);
  // Introduced with Power 8:
  // Data Stream Control Register
  inline void mtdscr_PPC(Register s1);
  inline void mfdscr_PPC(Register d );
  // Transactional Memory Registers
  inline void mftfhar_PPC(Register d);
  inline void mftfiar_PPC(Register d);
  inline void mftexasr_PPC(Register d);
  inline void mftexasru_PPC(Register d);

  // TEXASR bit description
  enum transaction_failure_reason {
    // Upper half (TEXASRU):
    tm_failure_code       =  0, // The Failure Code is copied from tabort or treclaim operand.
    tm_failure_persistent =  7, // The failure is likely to recur on each execution.
    tm_disallowed         =  8, // The instruction is not permitted.
    tm_nesting_of         =  9, // The maximum transaction level was exceeded.
    tm_footprint_of       = 10, // The tracking limit for transactional storage accesses was exceeded.
    tm_self_induced_cf    = 11, // A self-induced conflict occurred in Suspended state.
    tm_non_trans_cf       = 12, // A conflict occurred with a non-transactional access by another processor.
    tm_trans_cf           = 13, // A conflict occurred with another transaction.
    tm_translation_cf     = 14, // A conflict occurred with a TLB invalidation.
    tm_inst_fetch_cf      = 16, // An instruction fetch was performed from a block that was previously written transactionally.
    tm_tabort             = 31, // Termination was caused by the execution of an abort instruction.
    // Lower half:
    tm_suspended          = 32, // Failure was recorded in Suspended state.
    tm_failure_summary    = 36, // Failure has been detected and recorded.
    tm_tfiar_exact        = 37, // Value in the TFIAR is exact.
    tm_rot                = 38, // Rollback-only transaction.
    tm_transaction_level  = 52, // Transaction level (nesting depth + 1).
  };

  // RISCV 1, section 2.4.1 Branch Instructions
  inline void b_PPC(  address a, relocInfo::relocType rt = relocInfo::none);
  inline void b_PPC(  Label& L);
  inline void bl_PPC( address a, relocInfo::relocType rt = relocInfo::none);
  inline void bl_PPC( Label& L);
  inline void bc_PPC( int boint, int biint, address a, relocInfo::relocType rt = relocInfo::none);
  inline void bc_PPC( int boint, int biint, Label& L);
  inline void bcl_PPC(int boint, int biint, address a, relocInfo::relocType rt = relocInfo::none);
  inline void bcl_PPC(int boint, int biint, Label& L);

  inline void bclr_PPC(  int boint, int biint, int bhint, relocInfo::relocType rt = relocInfo::none);
  inline void bclrl_PPC( int boint, int biint, int bhint, relocInfo::relocType rt = relocInfo::none);
  inline void bcctr_PPC( int boint, int biint, int bhint = bhintbhBCCTRisNotReturnButSame,
                         relocInfo::relocType rt = relocInfo::none);
  inline void bcctrl_PPC(int boint, int biint, int bhint = bhintbhBCLRisReturn,
                         relocInfo::relocType rt = relocInfo::none);

  // helper function for b, bcxx
  inline bool is_within_range_of_b(address a, address pc);
  inline bool is_within_range_of_bcxx(address a, address pc);

  // get the destination of a bxx branch (b, bl, ba, bla)
  static inline address  bxx_destination_PPC(address baddr);
  static inline address  bxx_destination_PPC(int instr, address pc);
  static inline intptr_t bxx_destination_offset(int instr, intptr_t bxx_pos);

  // extended mnemonics for branch instructions
  inline void blt_PPC(ConditionRegister crx, Label& L);
  inline void bgt_PPC(ConditionRegister crx, Label& L);
  inline void beq_PPC(ConditionRegister crx, Label& L);
  inline void bso_PPC(ConditionRegister crx, Label& L);
  inline void bge_PPC(ConditionRegister crx, Label& L);
  inline void ble_PPC(ConditionRegister crx, Label& L);
  inline void bne_PPC(ConditionRegister crx, Label& L);
  inline void bns_PPC(ConditionRegister crx, Label& L);

  // Branch instructions with static prediction hints.
  inline void blt_predict_taken_PPC(    ConditionRegister crx, Label& L);
  inline void bgt_predict_taken_PPC(    ConditionRegister crx, Label& L);
  inline void beq_predict_taken_PPC(    ConditionRegister crx, Label& L);
  inline void bso_predict_taken_PPC(    ConditionRegister crx, Label& L);
  inline void bge_predict_taken_PPC(    ConditionRegister crx, Label& L);
  inline void ble_predict_taken_PPC(    ConditionRegister crx, Label& L);
  inline void bne_predict_taken_PPC(    ConditionRegister crx, Label& L);
  inline void bns_predict_taken_PPC(    ConditionRegister crx, Label& L);
  inline void blt_predict_not_taken_PPC(ConditionRegister crx, Label& L);
  inline void bgt_predict_not_taken_PPC(ConditionRegister crx, Label& L);
  inline void beq_predict_not_taken_PPC(ConditionRegister crx, Label& L);
  inline void bso_predict_not_taken_PPC(ConditionRegister crx, Label& L);
  inline void bge_predict_not_taken_PPC(ConditionRegister crx, Label& L);
  inline void ble_predict_not_taken_PPC(ConditionRegister crx, Label& L);
  inline void bne_predict_not_taken_PPC(ConditionRegister crx, Label& L);
  inline void bns_predict_not_taken_PPC(ConditionRegister crx, Label& L);

  // for use in conjunction with testbitdi:
  inline void btrue_PPC( ConditionRegister crx, Label& L);
  inline void bfalse_PPC(ConditionRegister crx, Label& L);

  inline void bltl_PPC(ConditionRegister crx, Label& L);
  inline void bgtl_PPC(ConditionRegister crx, Label& L);
  inline void beql_PPC(ConditionRegister crx, Label& L);
  inline void bsol_PPC(ConditionRegister crx, Label& L);
  inline void bgel_PPC(ConditionRegister crx, Label& L);
  inline void blel_PPC(ConditionRegister crx, Label& L);
  inline void bnel_PPC(ConditionRegister crx, Label& L);
  inline void bnsl_PPC(ConditionRegister crx, Label& L);

  // extended mnemonics for Branch Instructions via LR
  // We use `blr' for returns.
  inline void blr_PPC(relocInfo::relocType rt = relocInfo::none);

  // extended mnemonics for Branch Instructions with CTR
  // bdnz means `decrement CTR and jump to L if CTR is not zero'
  inline void bdnz_PPC(Label& L);
  // Decrement and branch if result is zero.
  inline void bdz_PPC(Label& L);
  // we use `bctr[l]' for jumps/calls in function descriptor glue
  // code, e.g. calls to runtime functions
  inline void bctr_PPC( relocInfo::relocType rt = relocInfo::none);
  inline void bctrl_PPC(relocInfo::relocType rt = relocInfo::none);
  // conditional jumps/branches via CTR
  inline void beqctr_PPC( ConditionRegister crx, relocInfo::relocType rt = relocInfo::none);
  inline void beqctrl_PPC(ConditionRegister crx, relocInfo::relocType rt = relocInfo::none);
  inline void bnectr_PPC( ConditionRegister crx, relocInfo::relocType rt = relocInfo::none);
  inline void bnectrl_PPC(ConditionRegister crx, relocInfo::relocType rt = relocInfo::none);

  // condition register logic instructions
  // NOTE: There's a preferred form: d and s2 should point into the same condition register.
  inline void crand_PPC( int d, int s1, int s2);
  inline void crnand_PPC(int d, int s1, int s2);
  inline void cror_PPC(  int d, int s1, int s2);
  inline void crxor_PPC( int d, int s1, int s2);
  inline void crnor_PPC( int d, int s1, int s2);
  inline void creqv_PPC( int d, int s1, int s2);
  inline void crandc_PPC(int d, int s1, int s2);
  inline void crorc_PPC( int d, int s1, int s2);

  // More convenient version.
  int condition_register_bit(ConditionRegister cr, Condition c) {
    return 4 * (int)(intptr_t)cr + c;
  }
  void crand_PPC( ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc);
  void crnand_PPC(ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc);
  void cror_PPC(  ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc);
  void crxor_PPC( ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc);
  void crnor_PPC( ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc);
  void creqv_PPC( ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc);
  void crandc_PPC(ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc);
  void crorc_PPC( ConditionRegister crdst, Condition cdst, ConditionRegister crsrc, Condition csrc);

  // icache and dcache related instructions
  inline void icbi_PPC(  Register s1, Register s2);
  //inline void dcba(Register s1, Register s2); // Instruction for embedded processor only.
  inline void dcbz_PPC(  Register s1, Register s2);
  inline void dcbst_PPC( Register s1, Register s2);
  inline void dcbf_PPC(  Register s1, Register s2);

  enum ct_cache_specification {
    ct_primary_cache   = 0,
    ct_secondary_cache = 2
  };
  // dcache read hint
  inline void dcbt_PPC(    Register s1, Register s2);
  inline void dcbtct_PPC(  Register s1, Register s2, int ct);
  inline void dcbtds_PPC(  Register s1, Register s2, int ds);
  // dcache write hint
  inline void dcbtst_PPC(  Register s1, Register s2);
  inline void dcbtstct_PPC(Register s1, Register s2, int ct);

  //  machine barrier instructions:
  //
  //  - sync    two-way memory barrier, aka fence
  //  - lwsync  orders  Store|Store,
  //                     Load|Store,
  //                     Load|Load,
  //            but not Store|Load
  //  - eieio   orders memory accesses for device memory (only)
  //  - isync   invalidates speculatively executed instructions
  //            From the Power ISA 2.06 documentation:
  //             "[...] an isync instruction prevents the execution of
  //            instructions following the isync until instructions
  //            preceding the isync have completed, [...]"
  //            From IBM's AIX assembler reference:
  //             "The isync [...] instructions causes the processor to
  //            refetch any instructions that might have been fetched
  //            prior to the isync instruction. The instruction isync
  //            causes the processor to wait for all previous instructions
  //            to complete. Then any instructions already fetched are
  //            discarded and instruction processing continues in the
  //            environment established by the previous instructions."
  //
  //  semantic barrier instructions:
  //  (as defined in orderAccess.hpp)
  //
  //  - release  orders Store|Store,       (maps to lwsync)
  //                     Load|Store
  //  - acquire  orders  Load|Store,       (maps to lwsync)
  //                     Load|Load
  //  - fence    orders Store|Store,       (maps to sync)
  //                     Load|Store,
  //                     Load|Load,
  //                    Store|Load
  //
 private:
  inline void sync_PPC(int l);
 public:
  inline void sync_PPC();
  inline void lwsync_PPC();
  inline void ptesync_PPC();
  inline void eieio_PPC();
  inline void isync_PPC();
  inline void elemental_membar_PPC(int e); // Elemental Memory Barriers (>=Power 8)

  // Wait instructions for polling. Attention: May result in SIGILL.
  inline void wait_PPC();
  inline void waitrsv_PPC(); // >=Power7

  // atomics
  inline void lbarx_unchecked_PPC(Register d, Register a, Register b, int eh1 = 0); // >=Power 8
  inline void lharx_unchecked_PPC(Register d, Register a, Register b, int eh1 = 0); // >=Power 8
  inline void lwarx_unchecked_PPC(Register d, Register a, Register b, int eh1 = 0);
  inline void ldarx_unchecked_PPC(Register d, Register a, Register b, int eh1 = 0);
  inline void lqarx_unchecked_PPC(Register d, Register a, Register b, int eh1 = 0); // >=Power 8
  inline bool lxarx_hint_exclusive_access_PPC();
  inline void lbarx_PPC(  Register d, Register a, Register b, bool hint_exclusive_access = false);
  inline void lharx_PPC(  Register d, Register a, Register b, bool hint_exclusive_access = false);
  inline void lwarx_PPC(  Register d, Register a, Register b, bool hint_exclusive_access = false);
  inline void ldarx_PPC(  Register d, Register a, Register b, bool hint_exclusive_access = false);
  inline void lqarx_PPC(  Register d, Register a, Register b, bool hint_exclusive_access = false);
  inline void stbcx__PPC( Register s, Register a, Register b);
  inline void sthcx__PPC( Register s, Register a, Register b);
  inline void stwcx__PPC( Register s, Register a, Register b);
  inline void stdcx__PPC( Register s, Register a, Register b);
  inline void stqcx__PPC( Register s, Register a, Register b);

  // Instructions for adjusting thread priority for simultaneous
  // multithreading (SMT) on Power5.
 private:
  inline void smt_prio_very_low_PPC();
  inline void smt_prio_medium_high_PPC();
  inline void smt_prio_high_PPC();

 public:
  inline void smt_prio_low_PPC();
  inline void smt_prio_medium_low_PPC();
  inline void smt_prio_medium_PPC();
  // >= Power7
  inline void smt_yield_PPC();
  inline void smt_mdoio_PPC();
  inline void smt_mdoom_PPC();
  // >= Power8
  inline void smt_miso_PPC();

  // trap instructions
  inline void twi_0_PPC(Register a); // for load with acquire semantics use load+twi_0+isync (trap can't occur)
  // NOT FOR DIRECT USE!!
 protected:
  inline void tdi_unchecked_PPC(int tobits, Register a, int si16);
  inline void twi_unchecked_PPC(int tobits, Register a, int si16);
  inline void tdi_PPC(          int tobits, Register a, int si16);   // asserts UseSIGTRAP
  inline void twi_PPC(          int tobits, Register a, int si16);   // asserts UseSIGTRAP
  inline void td_PPC(           int tobits, Register a, Register b); // asserts UseSIGTRAP
  inline void tw_PPC(           int tobits, Register a, Register b); // asserts UseSIGTRAP

  static bool is_tdi(int x, int tobits, int ra, int si16) {
     return (TDI_OPCODE == (x & TDI_OPCODE_MASK))
         && (tobits == inv_to_field(x))
         && (ra == -1/*any reg*/ || ra == inv_ra_field(x))
         && (si16 == inv_si_field(x));
  }

  static bool is_twi(int x, int tobits, int ra, int si16) {
     return (TWI_OPCODE == (x & TWI_OPCODE_MASK))
         && (tobits == inv_to_field(x))
         && (ra == -1/*any reg*/ || ra == inv_ra_field(x))
         && (si16 == inv_si_field(x));
  }

  static bool is_twi(int x, int tobits, int ra) {
     return (TWI_OPCODE == (x & TWI_OPCODE_MASK))
         && (tobits == inv_to_field(x))
         && (ra == -1/*any reg*/ || ra == inv_ra_field(x));
  }

  static bool is_td(int x, int tobits, int ra, int rb) {
     return (TD_OPCODE == (x & TD_OPCODE_MASK))
         && (tobits == inv_to_field(x))
         && (ra == -1/*any reg*/ || ra == inv_ra_field(x))
         && (rb == -1/*any reg*/ || rb == inv_rb_field(x));
  }

  static bool is_tw(int x, int tobits, int ra, int rb) {
     return (TW_OPCODE == (x & TW_OPCODE_MASK))
         && (tobits == inv_to_field(x))
         && (ra == -1/*any reg*/ || ra == inv_ra_field(x))
         && (rb == -1/*any reg*/ || rb == inv_rb_field(x));
  }

 public:
  // RISCV instructions

  // Issue an illegal instruction.
  inline void illtrap();
  static inline bool is_illtrap(int x);

  // Generic instructions
  inline void op_imm(Register d, Register s, int f, int imm);
  inline void lui(Register d, int imm);
  inline void auipc(Register d, int imm);
  inline void op(Register d, Register s1, Register s2, int f1, int f2);
  inline void amo(Register d, Register s1, Register s2, int f1, int f2, bool aq, bool rl);
  inline void jal(Register d, int off);
  inline void jalr(Register d, Register base, int off);
  inline void branch(Register s1, Register s2, int f, int off);
  inline void load(Register d, Register s, int width, int off);
  inline void load_fp(FloatRegister d, Register s, int width, int off);
  inline void store(Register s, Register base, int width, int off);
  inline void store_fp(FloatRegister s, Register base, int width, int off);
  inline void op_imm32(Register d, Register s, int f, int imm);
  inline void op32(Register d, Register s1, Register s2, int f1, int f2);
  inline void op_fp(Register d, Register s1, Register s2, int rm, int f);
  inline void op_fp(Register d, Register s1, int s2, int rm, int f);
  inline void op_fp(FloatRegister d, Register s1, int s2, int rm, int f);
  inline void op_fp(Register d, FloatRegister s1, int s2, int rm, int f);
  inline void madd(Register d, Register s1, Register s2, Register s3, int rm, int f);
  inline void msub(Register d, Register s1, Register s2, Register s3, int rm, int f);
  inline void nmadd(Register d, Register s1, Register s2, Register s3, int rm, int f);
  inline void nmsub(Register d, Register s1, Register s2, Register s3, int rm, int f);

  // Concrete instructions
  // op_imm
  inline void addi(    Register d, Register s, int imm);
  inline void slti(    Register d, Register s, int imm);
  inline void sltiu(   Register d, Register s, int imm);
  inline void xori(    Register d, Register s, int imm);
  inline void ori(     Register d, Register s, int imm);
  inline void andi(    Register d, Register s, int imm);
  inline void slli(    Register d, Register s, int shamt);
  inline void srli(    Register d, Register s, int shamt);
  inline void srai(    Register d, Register s, int shamt);
  // op
  inline void add(     Register d, Register s1, Register s2);
  inline void slt(     Register d, Register s1, Register s2);
  inline void sltu(    Register d, Register s1, Register s2);
  inline void andr(    Register d, Register s1, Register s2); // and is a C++ keyword
  inline void orr(     Register d, Register s1, Register s2); // or is a C++ keyword
  inline void xorr(    Register d, Register s1, Register s2); // xor is a C++ keyword
  inline void sll(     Register d, Register s1, Register s2);
  inline void srl(     Register d, Register s1, Register s2);
  inline void sub(     Register d, Register s1, Register s2);
  inline void sra(     Register d, Register s1, Register s2);
  inline void mul(     Register d, Register s1, Register s2);
  inline void mulh(    Register d, Register s1, Register s2);
  inline void mulhsu(  Register d, Register s1, Register s2);
  inline void mulhu(   Register d, Register s1, Register s2);
  inline void div(     Register d, Register s1, Register s2);
  inline void divu(    Register d, Register s1, Register s2);
  inline void rem(     Register d, Register s1, Register s2);
  inline void remu(    Register d, Register s1, Register s2);
  // branch
  inline void beq(     Register s1, Register s2, int off);
  inline void bne(     Register s1, Register s2, int off);
  inline void blt(     Register s1, Register s2, int off);
  inline void bltu(    Register s1, Register s2, int off);
  inline void bge(     Register s1, Register s2, int off);
  inline void bgeu(    Register s1, Register s2, int off);
  inline void beqz(    Register s,               int off);
  inline void bnez(    Register s,               int off);
  inline void beq(     Register s1, Register s2, Label& L);
  inline void bne(     Register s1, Register s2, Label& L);
  inline void beqz(    Register s,               Label& L);
  inline void bnez(    Register s,               Label& L);
  // load
  inline void ld(      Register d, Register s, int off);
  inline void lw(      Register d, Register s, int off);
  inline void lwu(     Register d, Register s, int off);
  inline void lh(      Register d, Register s, int off);
  inline void lhu(     Register d, Register s, int off);
  inline void lb(      Register d, Register s, int off);
  inline void lbu(     Register d, Register s, int off);
  // store
  inline void sd(      Register s, Register base, int off);
  inline void sw(      Register s, Register base, int off);
  inline void sh(      Register s, Register base, int off);
  inline void sb(      Register s, Register base, int off);
  // system
  inline void ecall();
  inline void ebreak();
  // op_imm32
  inline void addiw(   Register d, Register s, int imm);
  inline void slliw(   Register d, Register s, int shamt);
  inline void srliw(   Register d, Register s, int shamt);
  inline void sraiw(   Register d, Register s, int shamt);
  // op
  inline void addw(    Register d, Register s1, Register s2);
  inline void subw(    Register d, Register s1, Register s2);
  inline void sllw(    Register d, Register s1, Register s2);
  inline void srlw(    Register d, Register s1, Register s2);
  inline void sraw(    Register d, Register s1, Register s2);
  inline void mulw(    Register d, Register s1, Register s2);
  inline void divw(    Register d, Register s1, Register s2);
  inline void divuw(   Register d, Register s1, Register s2);
  inline void remw(    Register d, Register s1, Register s2);
  inline void remuw(   Register d, Register s1, Register s2);
  // amo
  inline void lrw(     Register d, Register s1,              bool aq, bool rl);
  inline void scw(     Register d, Register s1, Register s2, bool aq, bool rl);
  inline void amoswapw(Register d, Register s1, Register s2, bool aq, bool rl);
  inline void amoaddw( Register d, Register s1, Register s2, bool aq, bool rl);
  inline void amoxorw( Register d, Register s1, Register s2, bool aq, bool rl);
  inline void amoandw( Register d, Register s1, Register s2, bool aq, bool rl);
  inline void amoorw(  Register d, Register s1, Register s2, bool aq, bool rl);
  inline void amominw( Register d, Register s1, Register s2, bool aq, bool rl);
  inline void amomaxw( Register d, Register s1, Register s2, bool aq, bool rl);
  inline void amominuw(Register d, Register s1, Register s2, bool aq, bool rl);
  inline void amomaxuw(Register d, Register s1, Register s2, bool aq, bool rl);
  inline void lrd(     Register d, Register s1,              bool aq, bool rl);
  inline void scd(     Register d, Register s1, Register s2, bool aq, bool rl);
  inline void amoswapd(Register d, Register s1, Register s2, bool aq, bool rl);
  inline void amoaddd( Register d, Register s1, Register s2, bool aq, bool rl);
  inline void amoxord( Register d, Register s1, Register s2, bool aq, bool rl);
  inline void amoandd( Register d, Register s1, Register s2, bool aq, bool rl);
  inline void amoord(  Register d, Register s1, Register s2, bool aq, bool rl);
  inline void amomind( Register d, Register s1, Register s2, bool aq, bool rl);
  inline void amomaxd( Register d, Register s1, Register s2, bool aq, bool rl);
  inline void amominud(Register d, Register s1, Register s2, bool aq, bool rl);
  inline void amomaxud(Register d, Register s1, Register s2, bool aq, bool rl);
  //sp fp
  inline void flw(     FloatRegister d, Register s1, int imm);
  inline void fsw(     FloatRegister s, Register base, int imm);
  inline void fmadds(  Register d, Register s1, Register s2, Register s3, int rm);
  inline void fmsubs(  Register d, Register s1, Register s2, Register s3, int rm);
  inline void fnmadds( Register d, Register s1, Register s2, Register s3, int rm);
  inline void fnmsubs( Register d, Register s1, Register s2, Register s3, int rm);
  inline void fadds(   Register d, Register s1, Register s2, int rm);
  inline void fsubs(   Register d, Register s1, Register s2, int rm);
  inline void fmuls(   Register d, Register s1, Register s2, int rm);
  inline void fdivs(   Register d, Register s1, Register s2, int rm);
  inline void fsqrts(  Register d, Register s, int rm);
  inline void fsgnjs(  Register d, Register s1, Register s2);
  inline void fsgnjns( Register d, Register s1, Register s2);
  inline void fsgnjxs( Register d, Register s1, Register s2);
  inline void fmins(   Register d, Register s1, Register s2);
  inline void fmaxs(   Register d, Register s1, Register s2);
  inline void fcvtws(  Register d, Register s, int rm);
  inline void fcvtwus( Register d, Register s, int rm);
  inline void fmvxw(   Register d, Register s);
  inline void feqs(    Register d, Register s1, Register s2);
  inline void flts(    Register d, Register s1, Register s2);
  inline void fles(    Register d, Register s1, Register s2);
  inline void fclasss( Register d, Register s);
  inline void fcvtsw(  Register d, Register s, int rm);
  inline void fcvtswu( Register d, Register s, int rm);
  inline void fmvwx(   Register d, Register s);
  inline void fcvtls(  Register d, Register s, int rm);
  inline void fcvtlus( Register d, Register s, int rm);
  inline void fcvtsl(  Register d, Register s, int rm);
  inline void fcvtslu( Register d, Register s, int rm);
  //dp fp
  inline void fld(     FloatRegister d, Register s1, int imm);
  inline void fsd(     FloatRegister s, Register base, int imm);
  inline void fmaddd(  Register d, Register s1, Register s2, Register s3, int rm);
  inline void fmsubd(  Register d, Register s1, Register s2, Register s3, int rm);
  inline void fnmaddd( Register d, Register s1, Register s2, Register s3, int rm);
  inline void fnmsubd( Register d, Register s1, Register s2, Register s3, int rm);
  inline void faddd(   Register d, Register s1, Register s2, int rm);
  inline void fsubd(   Register d, Register s1, Register s2, int rm);
  inline void fmuld(   Register d, Register s1, Register s2, int rm);
  inline void fdivd(   Register d, Register s1, Register s2, int rm);
  inline void fsqrtd(  Register d, Register s, int rm);
  inline void fsgnjd(  Register d, Register s1, Register s2);
  inline void fsgnjnd( Register d, Register s1, Register s2);
  inline void fsgnjxd( Register d, Register s1, Register s2);
  inline void fmind(   Register d, Register s1, Register s2);
  inline void fmaxd(   Register d, Register s1, Register s2);
  inline void fcvtsd(  Register d, Register s, int rm);
  inline void fcvtds(  Register d, Register s, int rm);
  inline void feqd(    Register d, Register s1, Register s2);
  inline void fltd(    Register d, Register s1, Register s2);
  inline void fled(    Register d, Register s1, Register s2);
  inline void fclassd( Register d, Register s);
  inline void fcvtwd(  Register d, Register s, int rm);
  inline void fcvtwud( Register d, Register s, int rm);
  inline void fcvtdw(  Register d, Register s, int rm);
  inline void fcvtdwu( Register d, Register s, int rm);
  inline void fcvtld(  Register d, Register s, int rm);
  inline void fcvtlud( Register d, Register s, int rm);
  inline void fcvtdl(  Register d, Register s, int rm);
  inline void fcvtdlu( Register d, Register s, int rm);
  inline void fmvxd(   Register d, FloatRegister s);
  inline void fmvdx(   FloatRegister d, Register s);
  //qp fp
  inline void flq(     FloatRegister d, Register s1, int imm);
  inline void fsq(     FloatRegister s, Register base, int imm);
  inline void fmaddq(  Register d, Register s1, Register s2, Register s3, int rm);
  inline void fmsubq(  Register d, Register s1, Register s2, Register s3, int rm);
  inline void fnmaddq( Register d, Register s1, Register s2, Register s3, int rm);
  inline void fnmsubq( Register d, Register s1, Register s2, Register s3, int rm);
  inline void faddq(   Register d, Register s1, Register s2, int rm);
  inline void fsubq(   Register d, Register s1, Register s2, int rm);
  inline void fmulq(   Register d, Register s1, Register s2, int rm);
  inline void fdivq(   Register d, Register s1, Register s2, int rm);
  inline void fsqrtq(  Register d, Register s, int rm);
  inline void fsgnjq(  Register d, Register s1, Register s2);
  inline void fsgnjnq( Register d, Register s1, Register s2);
  inline void fsgnjxq( Register d, Register s1, Register s2);
  inline void fminq(   Register d, Register s1, Register s2);
  inline void fmaxq(   Register d, Register s1, Register s2);
  inline void fcvtsq(  Register d, Register s, int rm);
  inline void fcvtqs(  Register d, Register s, int rm);
  inline void fcvtdq(  Register d, Register s, int rm);
  inline void fcvtqd(  Register d, Register s, int rm);
  inline void feqq(    Register d, Register s1, Register s2);
  inline void fltq(    Register d, Register s1, Register s2);
  inline void fleq(    Register d, Register s1, Register s2);
  inline void fclassq( Register d, Register s);
  inline void fcvtwq(  Register d, Register s, int rm);
  inline void fcvtwuq( Register d, Register s, int rm);
  inline void fcvtqw(  Register d, Register s, int rm);
  inline void fcvtqwu( Register d, Register s, int rm);
  inline void fcvtlq(  Register d, Register s, int rm);
  inline void fcvtluq( Register d, Register s, int rm);
  inline void fcvtql(  Register d, Register s, int rm);
  inline void fcvtqlu( Register d, Register s, int rm);
  // pseudoinstructions
  inline void nop();
  inline void j(int off);
  inline void jal(int off);
  inline void jr(Register s);
  inline void jalr(Register s);
  inline void ret();
  inline void call(int off);
  inline void tail(int off);
  inline void neg(Register d, Register s);
  inline void mv(Register d, Register s);

private:
  bool li_32_RV(Register d, long long imm);
public:
  void li_RV(Register d, long long imm);

  // --- PPC instructions follow ---

  // RISCV floating point instructions
  // RISCV 1, section 4.6.2 Floating-Point Load Instructions
  inline void lfs_PPC(  FloatRegister d, int si16,   Register a);
  inline void lfsu_PPC( FloatRegister d, int si16,   Register a);
  inline void lfsx_PPC( FloatRegister d, Register a, Register b);
  inline void lfd_PPC(  FloatRegister d, int si16,   Register a);
  inline void lfdu_PPC( FloatRegister d, int si16,   Register a);
  inline void lfdx_PPC( FloatRegister d, Register a, Register b);

  // RISCV 1, section 4.6.3 Floating-Point Store Instructions
  inline void stfs_PPC(  FloatRegister s, int si16,   Register a);
  inline void stfsu_PPC( FloatRegister s, int si16,   Register a);
  inline void stfsx_PPC( FloatRegister s, Register a, Register b);
  inline void stfd_PPC(  FloatRegister s, int si16,   Register a);
  inline void stfdu_PPC( FloatRegister s, int si16,   Register a);
  inline void stfdx_PPC( FloatRegister s, Register a, Register b);

  // RISCV 1, section 4.6.4 Floating-Point Move Instructions
  inline void fmr_PPC(  FloatRegister d, FloatRegister b);
  inline void fmr__PPC( FloatRegister d, FloatRegister b);

  //  inline void mffgpr( FloatRegister d, Register b);
  //  inline void mftgpr( Register d, FloatRegister b);
  inline void cmpb_PPC(   Register a, Register s, Register b);
  inline void popcntb_PPC(Register a, Register s);
  inline void popcntw_PPC(Register a, Register s);
  inline void popcntd_PPC(Register a, Register s);

  inline void fneg_PPC(  FloatRegister d, FloatRegister b);
  inline void fneg__PPC( FloatRegister d, FloatRegister b);
  inline void fabs_PPC(  FloatRegister d, FloatRegister b);
  inline void fabs__PPC( FloatRegister d, FloatRegister b);
  inline void fnabs_PPC( FloatRegister d, FloatRegister b);
  inline void fnabs__PPC(FloatRegister d, FloatRegister b);

  // RISCV 1, section 4.6.5.1 Floating-Point Elementary Arithmetic Instructions
  inline void fadd_PPC(  FloatRegister d, FloatRegister a, FloatRegister b);
  inline void fadd__PPC( FloatRegister d, FloatRegister a, FloatRegister b);
  inline void fadds_PPC( FloatRegister d, FloatRegister a, FloatRegister b);
  inline void fadds__PPC(FloatRegister d, FloatRegister a, FloatRegister b);
  inline void fsub_PPC(  FloatRegister d, FloatRegister a, FloatRegister b);
  inline void fsub__PPC( FloatRegister d, FloatRegister a, FloatRegister b);
  inline void fsubs_PPC( FloatRegister d, FloatRegister a, FloatRegister b);
  inline void fsubs__PPC(FloatRegister d, FloatRegister a, FloatRegister b);
  inline void fmul_PPC(  FloatRegister d, FloatRegister a, FloatRegister c);
  inline void fmul__PPC( FloatRegister d, FloatRegister a, FloatRegister c);
  inline void fmuls_PPC( FloatRegister d, FloatRegister a, FloatRegister c);
  inline void fmuls__PPC(FloatRegister d, FloatRegister a, FloatRegister c);
  inline void fdiv_PPC(  FloatRegister d, FloatRegister a, FloatRegister b);
  inline void fdiv__PPC( FloatRegister d, FloatRegister a, FloatRegister b);
  inline void fdivs_PPC( FloatRegister d, FloatRegister a, FloatRegister b);
  inline void fdivs__PPC(FloatRegister d, FloatRegister a, FloatRegister b);

  // Fused multiply-accumulate instructions.
  // WARNING: Use only when rounding between the 2 parts is not desired.
  // Some floating point tck tests will fail if used incorrectly.
  inline void fmadd_PPC(   FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b);
  inline void fmadd__PPC(  FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b);
  inline void fmadds_PPC(  FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b);
  inline void fmadds__PPC( FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b);
  inline void fmsub_PPC(   FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b);
  inline void fmsub__PPC(  FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b);
  inline void fmsubs_PPC(  FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b);
  inline void fmsubs__PPC( FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b);
  inline void fnmadd_PPC(  FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b);
  inline void fnmadd__PPC( FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b);
  inline void fnmadds_PPC( FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b);
  inline void fnmadds__PPC(FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b);
  inline void fnmsub_PPC(  FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b);
  inline void fnmsub__PPC( FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b);
  inline void fnmsubs_PPC( FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b);
  inline void fnmsubs__PPC(FloatRegister d, FloatRegister a, FloatRegister c, FloatRegister b);

  // RISCV 1, section 4.6.6 Floating-Point Rounding and Conversion Instructions
  inline void frsp_PPC(  FloatRegister d, FloatRegister b);
  inline void fctid_PPC( FloatRegister d, FloatRegister b);
  inline void fctidz_PPC(FloatRegister d, FloatRegister b);
  inline void fctiw_PPC( FloatRegister d, FloatRegister b);
  inline void fctiwz_PPC(FloatRegister d, FloatRegister b);
  inline void fcfid_PPC( FloatRegister d, FloatRegister b);
  inline void fcfids_PPC(FloatRegister d, FloatRegister b);

  // RISCV 1, section 4.6.7 Floating-Point Compare Instructions
  inline void fcmpu_PPC( ConditionRegister crx, FloatRegister a, FloatRegister b);

  inline void fsqrt_PPC( FloatRegister d, FloatRegister b);
  inline void fsqrts_PPC(FloatRegister d, FloatRegister b);

  // Vector instructions for >= Power6.
  inline void lvebx_PPC(    VectorRegister d, Register s1, Register s2);
  inline void lvehx_PPC(    VectorRegister d, Register s1, Register s2);
  inline void lvewx_PPC(    VectorRegister d, Register s1, Register s2);
  inline void lvx_PPC(      VectorRegister d, Register s1, Register s2);
  inline void lvxl_PPC(     VectorRegister d, Register s1, Register s2);
  inline void stvebx_PPC(   VectorRegister d, Register s1, Register s2);
  inline void stvehx_PPC(   VectorRegister d, Register s1, Register s2);
  inline void stvewx_PPC(   VectorRegister d, Register s1, Register s2);
  inline void stvx_PPC(     VectorRegister d, Register s1, Register s2);
  inline void stvxl_PPC(    VectorRegister d, Register s1, Register s2);
  inline void lvsl_PPC(     VectorRegister d, Register s1, Register s2);
  inline void lvsr_PPC(     VectorRegister d, Register s1, Register s2);
  inline void vpkpx_PPC(    VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vpkshss_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vpkswss_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vpkshus_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vpkswus_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vpkuhum_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vpkuwum_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vpkuhus_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vpkuwus_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vupkhpx_PPC(  VectorRegister d, VectorRegister b);
  inline void vupkhsb_PPC(  VectorRegister d, VectorRegister b);
  inline void vupkhsh_PPC(  VectorRegister d, VectorRegister b);
  inline void vupklpx_PPC(  VectorRegister d, VectorRegister b);
  inline void vupklsb_PPC(  VectorRegister d, VectorRegister b);
  inline void vupklsh_PPC(  VectorRegister d, VectorRegister b);
  inline void vmrghb_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmrghw_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmrghh_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmrglb_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmrglw_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmrglh_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsplt_PPC(    VectorRegister d, int ui4,          VectorRegister b);
  inline void vsplth_PPC(   VectorRegister d, int ui3,          VectorRegister b);
  inline void vspltw_PPC(   VectorRegister d, int ui2,          VectorRegister b);
  inline void vspltisb_PPC( VectorRegister d, int si5);
  inline void vspltish_PPC( VectorRegister d, int si5);
  inline void vspltisw_PPC( VectorRegister d, int si5);
  inline void vperm_PPC(    VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c);
  inline void vsel_PPC(     VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c);
  inline void vsl_PPC(      VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsldoi_PPC(   VectorRegister d, VectorRegister a, VectorRegister b, int ui4);
  inline void vslo_PPC(     VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsr_PPC(      VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsro_PPC(     VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vaddcuw_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vaddshs_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vaddsbs_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vaddsws_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vaddubm_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vadduwm_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vadduhm_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vaddudm_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vaddubs_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vadduws_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vadduhs_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vaddfp_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsubcuw_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsubshs_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsubsbs_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsubsws_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsububm_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsubuwm_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsubuhm_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsubudm_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsububs_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsubuws_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsubuhs_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsubfp_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmulesb_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmuleub_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmulesh_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmuleuh_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmulosb_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmuloub_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmulosh_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmulosw_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmulouh_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmuluwm_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmhaddshs_PPC(VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c);
  inline void vmhraddshs_PPC(VectorRegister d,VectorRegister a, VectorRegister b, VectorRegister c);
  inline void vmladduhm_PPC(VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c);
  inline void vmsubuhm_PPC( VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c);
  inline void vmsummbm_PPC( VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c);
  inline void vmsumshm_PPC( VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c);
  inline void vmsumshs_PPC( VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c);
  inline void vmsumuhm_PPC( VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c);
  inline void vmsumuhs_PPC( VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c);
  inline void vmaddfp_PPC(  VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c);
  inline void vsumsws_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsum2sws_PPC( VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsum4sbs_PPC( VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsum4ubs_PPC( VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsum4shs_PPC( VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vavgsb_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vavgsw_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vavgsh_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vavgub_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vavguw_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vavguh_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmaxsb_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmaxsw_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmaxsh_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmaxub_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmaxuw_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmaxuh_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vminsb_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vminsw_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vminsh_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vminub_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vminuw_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vminuh_PPC(   VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vcmpequb_PPC( VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vcmpequh_PPC( VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vcmpequw_PPC( VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vcmpgtsh_PPC( VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vcmpgtsb_PPC( VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vcmpgtsw_PPC( VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vcmpgtub_PPC( VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vcmpgtuh_PPC( VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vcmpgtuw_PPC( VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vcmpequb__PPC(VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vcmpequh__PPC(VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vcmpequw__PPC(VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vcmpgtsh__PPC(VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vcmpgtsb__PPC(VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vcmpgtsw__PPC(VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vcmpgtub__PPC(VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vcmpgtuh__PPC(VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vcmpgtuw__PPC(VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vand_PPC(     VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vandc_PPC(    VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vnor_PPC(     VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vor_PPC(      VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vmr_PPC(      VectorRegister d, VectorRegister a);
  inline void vxor_PPC(     VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vrld_PPC(     VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vrlb_PPC(     VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vrlw_PPC(     VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vrlh_PPC(     VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vslb_PPC(     VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vskw_PPC(     VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vslh_PPC(     VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsrb_PPC(     VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsrw_PPC(     VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsrh_PPC(     VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsrab_PPC(    VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsraw_PPC(    VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsrah_PPC(    VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vpopcntw_PPC( VectorRegister d, VectorRegister b);
  // Vector Floating-Point not implemented yet
  inline void mtvscr_PPC(   VectorRegister b);
  inline void mfvscr_PPC(   VectorRegister d);

  // Vector-Scalar (VSX) instructions.
  inline void lxvd2x_PPC(   VectorSRegister d, Register a);
  inline void lxvd2x_PPC(   VectorSRegister d, Register a, Register b);
  inline void stxvd2x_PPC(  VectorSRegister d, Register a);
  inline void stxvd2x_PPC(  VectorSRegister d, Register a, Register b);
  inline void mtvrwz_PPC(   VectorRegister  d, Register a);
  inline void mfvrwz_PPC(   Register        a, VectorRegister d);
  inline void mtvrd_PPC(    VectorRegister  d, Register a);
  inline void mfvrd_PPC(    Register        a, VectorRegister d);
  inline void xxpermdi_PPC( VectorSRegister d, VectorSRegister a, VectorSRegister b, int dm);
  inline void xxmrghw_PPC(  VectorSRegister d, VectorSRegister a, VectorSRegister b);
  inline void xxmrglw_PPC(  VectorSRegister d, VectorSRegister a, VectorSRegister b);
  inline void mtvsrd_PPC(   VectorSRegister d, Register a);
  inline void mtvsrwz_PPC(  VectorSRegister d, Register a);
  inline void xxspltw_PPC(  VectorSRegister d, VectorSRegister b, int ui2);
  inline void xxlor_PPC(    VectorSRegister d, VectorSRegister a, VectorSRegister b);
  inline void xxlxor_PPC(   VectorSRegister d, VectorSRegister a, VectorSRegister b);
  inline void xxleqv_PPC(   VectorSRegister d, VectorSRegister a, VectorSRegister b);
  inline void xvdivsp_PPC(  VectorSRegister d, VectorSRegister a, VectorSRegister b);
  inline void xvdivdp_PPC(  VectorSRegister d, VectorSRegister a, VectorSRegister b);
  inline void xvabssp_PPC(  VectorSRegister d, VectorSRegister b);
  inline void xvabsdp_PPC(  VectorSRegister d, VectorSRegister b);
  inline void xvnegsp_PPC(  VectorSRegister d, VectorSRegister b);
  inline void xvnegdp_PPC(  VectorSRegister d, VectorSRegister b);
  inline void xvsqrtsp_PPC( VectorSRegister d, VectorSRegister b);
  inline void xvsqrtdp_PPC( VectorSRegister d, VectorSRegister b);
  inline void xscvdpspn_PPC(VectorSRegister d, VectorSRegister b);
  inline void xvadddp_PPC(  VectorSRegister d, VectorSRegister a, VectorSRegister b);
  inline void xvsubdp_PPC(  VectorSRegister d, VectorSRegister a, VectorSRegister b);
  inline void xvmulsp_PPC(  VectorSRegister d, VectorSRegister a, VectorSRegister b);
  inline void xvmuldp_PPC(  VectorSRegister d, VectorSRegister a, VectorSRegister b);
  inline void xvmaddasp_PPC(VectorSRegister d, VectorSRegister a, VectorSRegister b);
  inline void xvmaddadp_PPC(VectorSRegister d, VectorSRegister a, VectorSRegister b);
  inline void xvmsubasp_PPC(VectorSRegister d, VectorSRegister a, VectorSRegister b);
  inline void xvmsubadp_PPC(VectorSRegister d, VectorSRegister a, VectorSRegister b);
  inline void xvnmsubasp_PPC(VectorSRegister d, VectorSRegister a, VectorSRegister b);
  inline void xvnmsubadp_PPC(VectorSRegister d, VectorSRegister a, VectorSRegister b);

  // VSX Extended Mnemonics
  inline void xxspltd_PPC(  VectorSRegister d, VectorSRegister a, int x);
  inline void xxmrghd_PPC(  VectorSRegister d, VectorSRegister a, VectorSRegister b);
  inline void xxmrgld_PPC(  VectorSRegister d, VectorSRegister a, VectorSRegister b);
  inline void xxswapd_PPC(  VectorSRegister d, VectorSRegister a);

  // Vector-Scalar (VSX) instructions.
  inline void mtfprd_PPC(   FloatRegister   d, Register a);
  inline void mtfprwa_PPC(  FloatRegister   d, Register a);
  inline void mffprd_PPC(   Register        a, FloatRegister d);

  // Deliver A Random Number (introduced with POWER9)
  inline void darn_PPC( Register d, int l = 1 /*L=CRN*/);

  // AES (introduced with Power 8)
  inline void vcipher_PPC(     VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vcipherlast_PPC( VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vncipher_PPC(    VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vncipherlast_PPC(VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vsbox_PPC(       VectorRegister d, VectorRegister a);

  // SHA (introduced with Power 8)
  inline void vshasigmad_PPC(VectorRegister d, VectorRegister a, bool st, int six);
  inline void vshasigmaw_PPC(VectorRegister d, VectorRegister a, bool st, int six);

  // Vector Binary Polynomial Multiplication (introduced with Power 8)
  inline void vpmsumb_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vpmsumd_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vpmsumh_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);
  inline void vpmsumw_PPC(  VectorRegister d, VectorRegister a, VectorRegister b);

  // Vector Permute and Xor (introduced with Power 8)
  inline void vpermxor_PPC( VectorRegister d, VectorRegister a, VectorRegister b, VectorRegister c);

  // Transactional Memory instructions (introduced with Power 8)
  inline void tbegin__PPC();    // R=0
  inline void tbeginrot__PPC(); // R=1 Rollback-Only Transaction
  inline void tend__PPC();    // A=0
  inline void tendall__PPC(); // A=1
  inline void tabort__PPC();
  inline void tabort__PPC(Register a);
  inline void tabortwc__PPC(int t, Register a, Register b);
  inline void tabortwci__PPC(int t, Register a, int si);
  inline void tabortdc__PPC(int t, Register a, Register b);
  inline void tabortdci__PPC(int t, Register a, int si);
  inline void tsuspend__PPC(); // tsr with L=0
  inline void tresume__PPC();  // tsr with L=1
  inline void tcheck_PPC(int f);

  static bool is_tbegin(int x) {
    return TBEGIN_OPCODE == (x & (0x3f << OPCODE_SHIFT | 0x3ff << 1));
  }

  // The following encoders use r0 as second operand. These instructions
  // read r0 as '0'.
  inline void lwzx_PPC( Register d, Register s2);
  inline void lwz_PPC(  Register d, int si16);
  inline void lwax_PPC( Register d, Register s2);
  inline void lwa_PPC(  Register d, int si16);
  inline void lwbrx_PPC(Register d, Register s2);
  inline void lhzx_PPC( Register d, Register s2);
  inline void lhz_PPC(  Register d, int si16);
  inline void lhax_PPC( Register d, Register s2);
  inline void lha_PPC(  Register d, int si16);
  inline void lhbrx_PPC(Register d, Register s2);
  inline void lbzx_PPC( Register d, Register s2);
  inline void lbz_PPC(  Register d, int si16);
  inline void ldx_PPC(  Register d, Register s2);
  inline void ld_PPC(   Register d, int si16);
  inline void ldbrx_PPC(Register d, Register s2);
  inline void stwx_PPC( Register d, Register s2);
  inline void stw_PPC(  Register d, int si16);
  inline void stwbrx_PPC( Register d, Register s2);
  inline void sthx_PPC( Register d, Register s2);
  inline void sth_PPC(  Register d, int si16);
  inline void sthbrx_PPC( Register d, Register s2);
  inline void stbx_PPC( Register d, Register s2);
  inline void stb_PPC(  Register d, int si16);
  inline void stdx_PPC( Register d, Register s2);
  inline void std_PPC(  Register d, int si16);
  inline void stdbrx_PPC( Register d, Register s2);

  // RISCV 2, section 3.2.1 Instruction Cache Instructions
  inline void icbi_PPC(    Register s2);
  // RISCV 2, section 3.2.2 Data Cache Instructions
  //inlinevoid dcba(   Register s2); // Instruction for embedded processor only.
  inline void dcbz_PPC(    Register s2);
  inline void dcbst_PPC(   Register s2);
  inline void dcbf_PPC(    Register s2);
  // dcache read hint
  inline void dcbt_PPC(    Register s2);
  inline void dcbtct_PPC(  Register s2, int ct);
  inline void dcbtds_PPC(  Register s2, int ds);
  // dcache write hint
  inline void dcbtst_PPC(  Register s2);
  inline void dcbtstct_PPC(Register s2, int ct);

  // Atomics: use ra0mem to disallow R0 as base.
  inline void lbarx_unchecked_PPC(Register d, Register b, int eh1);
  inline void lharx_unchecked_PPC(Register d, Register b, int eh1);
  inline void lwarx_unchecked_PPC(Register d, Register b, int eh1);
  inline void ldarx_unchecked_PPC(Register d, Register b, int eh1);
  inline void lqarx_unchecked_PPC(Register d, Register b, int eh1);
  inline void lbarx_PPC( Register d, Register b, bool hint_exclusive_access);
  inline void lharx_PPC( Register d, Register b, bool hint_exclusive_access);
  inline void lwarx_PPC( Register d, Register b, bool hint_exclusive_access);
  inline void ldarx_PPC( Register d, Register b, bool hint_exclusive_access);
  inline void lqarx_PPC( Register d, Register b, bool hint_exclusive_access);
  inline void stbcx__PPC(Register s, Register b);
  inline void sthcx__PPC(Register s, Register b);
  inline void stwcx__PPC(Register s, Register b);
  inline void stdcx__PPC(Register s, Register b);
  inline void stqcx__PPC(Register s, Register b);
  inline void lfs_PPC(   FloatRegister d, int si16);
  inline void lfsx_PPC(  FloatRegister d, Register b);
  inline void lfd_PPC(   FloatRegister d, int si16);
  inline void lfdx_PPC(  FloatRegister d, Register b);
  inline void stfs_PPC(  FloatRegister s, int si16);
  inline void stfsx_PPC( FloatRegister s, Register b);
  inline void stfd_PPC(  FloatRegister s, int si16);
  inline void stfdx_PPC( FloatRegister s, Register b);
  inline void lvebx_PPC( VectorRegister d, Register s2);
  inline void lvehx_PPC( VectorRegister d, Register s2);
  inline void lvewx_PPC( VectorRegister d, Register s2);
  inline void lvx_PPC(   VectorRegister d, Register s2);
  inline void lvxl_PPC(  VectorRegister d, Register s2);
  inline void stvebx_PPC(VectorRegister d, Register s2);
  inline void stvehx_PPC(VectorRegister d, Register s2);
  inline void stvewx_PPC(VectorRegister d, Register s2);
  inline void stvx_PPC(  VectorRegister d, Register s2);
  inline void stvxl_PPC( VectorRegister d, Register s2);
  inline void lvsl_PPC(  VectorRegister d, Register s2);
  inline void lvsr_PPC(  VectorRegister d, Register s2);

  // Endianess specific concatenation of 2 loaded vectors.
  inline void load_perm_PPC(VectorRegister perm, Register addr);
  inline void vec_perm_PPC(VectorRegister first_dest, VectorRegister second, VectorRegister perm);
  inline void vec_perm_PPC(VectorRegister dest, VectorRegister first, VectorRegister second, VectorRegister perm);

  // RegisterOrConstant versions.
  // These emitters choose between the versions using two registers and
  // those with register and immediate, depending on the content of roc.
  // If the constant is not encodable as immediate, instructions to
  // load the constant are emitted beforehand. Store instructions need a
  // tmp reg if the constant is not encodable as immediate.
  // Size unpredictable.
  void ld_PPC(  Register d, RegisterOrConstant roc, Register s1 = noreg);
  void lwa_PPC( Register d, RegisterOrConstant roc, Register s1 = noreg);
  void lwz_PPC( Register d, RegisterOrConstant roc, Register s1 = noreg);
  void lha_PPC( Register d, RegisterOrConstant roc, Register s1 = noreg);
  void lhz_PPC( Register d, RegisterOrConstant roc, Register s1 = noreg);
  void lbz_PPC( Register d, RegisterOrConstant roc, Register s1 = noreg);
  void std_PPC( Register d, RegisterOrConstant roc, Register s1 = noreg, Register tmp = noreg);
  void stw_PPC( Register d, RegisterOrConstant roc, Register s1 = noreg, Register tmp = noreg);
  void sth_PPC( Register d, RegisterOrConstant roc, Register s1 = noreg, Register tmp = noreg);
  void stb_PPC( Register d, RegisterOrConstant roc, Register s1 = noreg, Register tmp = noreg);
  void add_PPC( Register d, RegisterOrConstant roc, Register s1);
  void subf_PPC(Register d, RegisterOrConstant roc, Register s1);
  void cmpd_PPC(ConditionRegister d, RegisterOrConstant roc, Register s1);
  // Load pointer d from s1+roc.
  void ld_ptr_PPC(Register d, RegisterOrConstant roc, Register s1 = noreg) { ld_PPC(d, roc, s1); }

  // Emit several instructions to load a 64 bit constant. This issues a fixed
  // instruction pattern so that the constant can be patched later on.
  enum {
    load_const_size = 5 * BytesPerInstWord
  };
         void load_const_PPC(Register d, long a,            Register tmp = noreg);
  inline void load_const_PPC(Register d, void* a,           Register tmp = noreg);
  inline void load_const_PPC(Register d, Label& L,          Register tmp = noreg);
  inline void load_const_PPC(Register d, AddressLiteral& a, Register tmp = noreg);
  inline void load_const32_PPC(Register d, int i); // load signed int (patchable)

  // Load a 64 bit constant, optimized, not identifyable.
  // Tmp can be used to increase ILP. Set return_simm16_rest = true to get a
  // 16 bit immediate offset. This is useful if the offset can be encoded in
  // a succeeding instruction.
         int load_const_optimized(Register d, long a,  Register tmp = noreg, bool return_simm16_rest = false);
  inline int load_const_optimized(Register d, void* a, Register tmp = noreg, bool return_simm16_rest = false) {
    return load_const_optimized(d, (long)(unsigned long)a, tmp, return_simm16_rest);
  }

  // If return_simm16_rest, the return value needs to get added afterwards.
         int add_const_optimized(Register d, Register s, long x, Register tmp = R0, bool return_simm16_rest = false);
  inline int add_const_optimized(Register d, Register s, void* a, Register tmp = R0, bool return_simm16_rest = false) {
    return add_const_optimized(d, s, (long)(unsigned long)a, tmp, return_simm16_rest);
  }

  // If return_simm16_rest, the return value needs to get added afterwards.
  inline int sub_const_optimized(Register d, Register s, long x, Register tmp = R0, bool return_simm16_rest = false) {
    return add_const_optimized(d, s, -x, tmp, return_simm16_rest);
  }
  inline int sub_const_optimized(Register d, Register s, void* a, Register tmp = R0, bool return_simm16_rest = false) {
    return sub_const_optimized(d, s, (long)(unsigned long)a, tmp, return_simm16_rest);
  }

  // Creation
  Assembler(CodeBuffer* code) : AbstractAssembler(code) {
#ifdef CHECK_DELAY
    delay_state = no_delay;
#endif
  }

  // Testing
#ifndef PRODUCT
  void test_asm();
#endif
};


#endif // CPU_RISCV_ASSEMBLER_RISCV_HPP
