/*
 * Copyright (c) 1997, 2019, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, 2019, SAP SE. All rights reserved.
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

#include "precompiled.hpp"
#include "asm/macroAssembler.inline.hpp"
#include "compiler/disassembler.hpp"
#include "gc/shared/collectedHeap.inline.hpp"
#include "gc/shared/barrierSet.hpp"
#include "gc/shared/barrierSetAssembler.hpp"
#include "interpreter/interpreter.hpp"
#include "memory/resourceArea.hpp"
#include "nativeInst_riscv.hpp"
#include "prims/methodHandles.hpp"
#include "runtime/biasedLocking.hpp"
#include "runtime/icache.hpp"
#include "runtime/interfaceSupport.inline.hpp"
#include "runtime/objectMonitor.hpp"
#include "runtime/os.hpp"
#include "runtime/safepoint.hpp"
#include "runtime/safepointMechanism.hpp"
#include "runtime/sharedRuntime.hpp"
#include "runtime/stubRoutines.hpp"
#include "utilities/macros.hpp"
#ifdef COMPILER2
#include "opto/intrinsicnode.hpp"
#endif

#ifdef PRODUCT
#define BLOCK_COMMENT(str) // nothing
#else
#define BLOCK_COMMENT(str) block_comment(str)
#endif
#define BIND(label) bind(label); BLOCK_COMMENT(#label ":")

#ifdef ASSERT
// On RISC, there's no benefit to verifying instruction boundaries.
bool AbstractAssembler::pd_check_instruction_mark() { return false; }
#endif

void MacroAssembler::ld_largeoffset_unchecked(Register d, int si31, Register a, int emit_filler_nop) {
  assert(Assembler::is_simm(si31, 31) && si31 >= 0, "si31 out of range");
  if (Assembler::is_simm(si31, 16)) {
    ld_PPC(d, si31, a);
    if (emit_filler_nop) nop_PPC();
  } else {
    const int hi = MacroAssembler::largeoffset_si16_si16_hi(si31);
    const int lo = MacroAssembler::largeoffset_si16_si16_lo(si31);
    addis_PPC(d, a, hi);
    ld_PPC(d, lo, d);
  }
}

void MacroAssembler::ld_largeoffset(Register d, int si31, Register a, int emit_filler_nop) {
  assert_different_registers(d, a);
  ld_largeoffset_unchecked(d, si31, a, emit_filler_nop);
}

void MacroAssembler::load_sized_value(Register dst, RegisterOrConstant offs, Register base,
                                      size_t size_in_bytes, bool is_signed) {
  switch (size_in_bytes) {
  case  8:              ld_PPC(dst, offs, base);                         break;
  case  4:  is_signed ? lwa_PPC(dst, offs, base) : lwz_PPC(dst, offs, base); break;
  case  2:  is_signed ? lha_PPC(dst, offs, base) : lhz_PPC(dst, offs, base); break;
  case  1:  lbz_PPC(dst, offs, base); if (is_signed) extsb_PPC(dst, dst);    break; // lba doesn't exist :(
  default:  ShouldNotReachHere();
  }
}

void MacroAssembler::store_sized_value(Register dst, RegisterOrConstant offs, Register base,
                                       size_t size_in_bytes) {
  switch (size_in_bytes) {
  case  8:  std_PPC(dst, offs, base); break;
  case  4:  stw_PPC(dst, offs, base); break;
  case  2:  sth_PPC(dst, offs, base); break;
  case  1:  stb_PPC(dst, offs, base); break;
  default:  ShouldNotReachHere();
  }
}

void MacroAssembler::align(int modulus, int max, int rem) {
  // FIXME_RISCV:
  /*int padding = (rem + modulus - (offset() % modulus)) % modulus;
  if (padding > max) return;
  for (int c = (padding >> 2); c > 0; --c) { nop(); }*/
}

// Issue instructions that calculate given TOC from global TOC.
void MacroAssembler::calculate_address_from_global_toc(Register dst, address addr, bool hi16, bool lo16,
                                                       bool add_relocation, bool emit_dummy_addr) {
  int offset = -1;
  if (emit_dummy_addr) {
    offset = -128; // dummy address
  } else if (addr != (address)(intptr_t)-1) {
    offset = MacroAssembler::offset_to_global_toc(addr);
  }

  if (hi16) {
    addis_PPC(dst, R20_TOC, MacroAssembler::largeoffset_si16_si16_hi(offset));
  }
  if (lo16) {
    if (add_relocation) {
      // Relocate at the addi to avoid confusion with a load from the method's TOC.
      relocate(internal_word_Relocation::spec(addr));
    }
    addi_PPC(dst, dst, MacroAssembler::largeoffset_si16_si16_lo(offset));
  }
}

address MacroAssembler::patch_calculate_address_from_global_toc_at(address a, address bound, address addr) {
  const int offset = MacroAssembler::offset_to_global_toc(addr);

  const address inst2_addr = a;
  const int inst2 = *(int *)inst2_addr;

  // The relocation points to the second instruction, the addi,
  // and the addi reads and writes the same register dst.
  const int dst = inv_rt_field(inst2);
  assert(is_addi(inst2) && inv_ra_field(inst2) == dst, "must be addi reading and writing dst");

  // Now, find the preceding addis which writes to dst.
  int inst1 = 0;
  address inst1_addr = inst2_addr - BytesPerInstWord;
  while (inst1_addr >= bound) {
    inst1 = *(int *) inst1_addr;
    if (is_addis(inst1) && inv_rt_field(inst1) == dst) {
      // Stop, found the addis which writes dst.
      break;
    }
    inst1_addr -= BytesPerInstWord;
  }

  assert(is_addis(inst1) && inv_ra_field(inst1) == 29 /* R29 */, "source must be global TOC");
  set_imm((int *)inst1_addr, MacroAssembler::largeoffset_si16_si16_hi(offset));
  set_imm((int *)inst2_addr, MacroAssembler::largeoffset_si16_si16_lo(offset));
  return inst1_addr;
}

address MacroAssembler::get_address_of_calculate_address_from_global_toc_at(address a, address bound) {
  const address inst2_addr = a;
  const int inst2 = *(int *)inst2_addr;

  // The relocation points to the second instruction, the addi,
  // and the addi reads and writes the same register dst.
  const int dst = inv_rt_field(inst2);
  assert(is_addi(inst2) && inv_ra_field(inst2) == dst, "must be addi reading and writing dst");

  // Now, find the preceding addis which writes to dst.
  int inst1 = 0;
  address inst1_addr = inst2_addr - BytesPerInstWord;
  while (inst1_addr >= bound) {
    inst1 = *(int *) inst1_addr;
    if (is_addis(inst1) && inv_rt_field(inst1) == dst) {
      // stop, found the addis which writes dst
      break;
    }
    inst1_addr -= BytesPerInstWord;
  }

  assert(is_addis(inst1) && inv_ra_field(inst1) == 29 /* R29 */, "source must be global TOC");

  int offset = (get_imm(inst1_addr, 0) << 16) + get_imm(inst2_addr, 0);
  // -1 is a special case
  if (offset == -1) {
    return (address)(intptr_t)-1;
  } else {
    return global_toc() + offset;
  }
}

#ifdef _LP64
// Patch compressed oops or klass constants.
// Assembler sequence is
// 1) compressed oops:
//    lis  rx = const.hi
//    ori rx = rx | const.lo
// 2) compressed klass:
//    lis  rx = const.hi
//    clrldi rx = rx & 0xFFFFffff // clearMS32b, optional
//    ori rx = rx | const.lo
// Clrldi will be passed by.
address MacroAssembler::patch_set_narrow_oop(address a, address bound, narrowOop data) {
  assert(UseCompressedOops, "Should only patch compressed oops");

  const address inst2_addr = a;
  const int inst2 = *(int *)inst2_addr;

  // The relocation points to the second instruction, the ori,
  // and the ori reads and writes the same register dst.
  const int dst = inv_rta_field(inst2);
  assert(is_ori(inst2) && inv_rs_field(inst2) == dst, "must be ori reading and writing dst");
  // Now, find the preceding addis which writes to dst.
  int inst1 = 0;
  address inst1_addr = inst2_addr - BytesPerInstWord;
  bool inst1_found = false;
  while (inst1_addr >= bound) {
    inst1 = *(int *)inst1_addr;
    if (is_lis(inst1) && inv_rs_field(inst1) == dst) { inst1_found = true; break; }
    inst1_addr -= BytesPerInstWord;
  }
  assert(inst1_found, "inst is not lis");

  int xc = (data >> 16) & 0xffff;
  int xd = (data >>  0) & 0xffff;

  set_imm((int *)inst1_addr, (short)(xc)); // see enc_load_con_narrow_hi/_lo
  set_imm((int *)inst2_addr,        (xd)); // unsigned int
  return inst1_addr;
}

// Get compressed oop or klass constant.
narrowOop MacroAssembler::get_narrow_oop(address a, address bound) {
  assert(UseCompressedOops, "Should only patch compressed oops");

  const address inst2_addr = a;
  const int inst2 = *(int *)inst2_addr;

  // The relocation points to the second instruction, the ori,
  // and the ori reads and writes the same register dst.
  const int dst = inv_rta_field(inst2);
  assert(is_ori(inst2) && inv_rs_field(inst2) == dst, "must be ori reading and writing dst");
  // Now, find the preceding lis which writes to dst.
  int inst1 = 0;
  address inst1_addr = inst2_addr - BytesPerInstWord;
  bool inst1_found = false;

  while (inst1_addr >= bound) {
    inst1 = *(int *) inst1_addr;
    if (is_lis(inst1) && inv_rs_field(inst1) == dst) { inst1_found = true; break;}
    inst1_addr -= BytesPerInstWord;
  }
  assert(inst1_found, "inst is not lis");

  uint xl = ((unsigned int) (get_imm(inst2_addr, 0) & 0xffff));
  uint xh = (((get_imm(inst1_addr, 0)) & 0xffff) << 16);

  return (int) (xl | xh);
}
#endif // _LP64

// Returns true if successful.
bool MacroAssembler::load_const_from_method_toc(Register dst, AddressLiteral& a,
                                                Register toc, bool fixed_size) {
  int toc_offset = 0;
  // Use RelocationHolder::none for the constant pool entry, otherwise
  // we will end up with a failing NativeCall::verify(x) where x is
  // the address of the constant pool entry.
  // FIXME: We should insert relocation information for oops at the constant
  // pool entries instead of inserting it at the loads; patching of a constant
  // pool entry should be less expensive.
  address const_address = address_constant((address)a.value(), RelocationHolder::none);
  if (const_address == NULL) { return false; } // allocation failure
  // Relocate at the pc of the load.
  relocate(a.rspec());
  toc_offset = (int)(const_address - code()->consts()->start());
  ld_largeoffset_unchecked(dst, toc_offset, toc, fixed_size);
  return true;
}

bool MacroAssembler::is_load_const_from_method_toc_at(address a) {
  const address inst1_addr = a;
  const int inst1 = *(int *)inst1_addr;

   // The relocation points to the ld or the addis.
   return (is_ld(inst1)) ||
          (is_addis(inst1) && inv_ra_field(inst1) != 0);
}

int MacroAssembler::get_offset_of_load_const_from_method_toc_at(address a) {
  assert(is_load_const_from_method_toc_at(a), "must be load_const_from_method_toc");

  const address inst1_addr = a;
  const int inst1 = *(int *)inst1_addr;

  if (is_ld(inst1)) {
    return inv_d1_field(inst1);
  } else if (is_addis(inst1)) {
    const int dst = inv_rt_field(inst1);

    // Now, find the succeeding ld which reads and writes to dst.
    address inst2_addr = inst1_addr + BytesPerInstWord;
    int inst2 = 0;
    while (true) {
      inst2 = *(int *) inst2_addr;
      if (is_ld(inst2) && inv_ra_field(inst2) == dst && inv_rt_field(inst2) == dst) {
        // Stop, found the ld which reads and writes dst.
        break;
      }
      inst2_addr += BytesPerInstWord;
    }
    return (inv_d1_field(inst1) << 16) + inv_d1_field(inst2);
  }
  ShouldNotReachHere();
  return 0;
}

// Get the constant from a `load_const' sequence.
long MacroAssembler::get_const(address a) {
  assert(is_load_const_at(a), "not a load of a constant");
  const int *p = (const int*) a;
  unsigned long x = (((unsigned long) (get_imm(a,0) & 0xffff)) << 48);
  if (is_ori(*(p+1))) {
    x |= (((unsigned long) (get_imm(a,1) & 0xffff)) << 32);
    x |= (((unsigned long) (get_imm(a,3) & 0xffff)) << 16);
    x |= (((unsigned long) (get_imm(a,4) & 0xffff)));
  } else if (is_lis(*(p+1))) {
    x |= (((unsigned long) (get_imm(a,2) & 0xffff)) << 32);
    x |= (((unsigned long) (get_imm(a,1) & 0xffff)) << 16);
    x |= (((unsigned long) (get_imm(a,3) & 0xffff)));
  } else {
    ShouldNotReachHere();
    return (long) 0;
  }
  return (long) x;
}

// Patch the 64 bit constant of a `load_const' sequence. This is a low
// level procedure. It neither flushes the instruction cache nor is it
// mt safe.
void MacroAssembler::patch_const(address a, long x) {
  assert(is_load_const_at(a), "not a load of a constant");
  int *p = (int*) a;
  if (is_ori(*(p+1))) {
    set_imm(0 + p, (x >> 48) & 0xffff);
    set_imm(1 + p, (x >> 32) & 0xffff);
    set_imm(3 + p, (x >> 16) & 0xffff);
    set_imm(4 + p, x & 0xffff);
  } else if (is_lis(*(p+1))) {
    set_imm(0 + p, (x >> 48) & 0xffff);
    set_imm(2 + p, (x >> 32) & 0xffff);
    set_imm(1 + p, (x >> 16) & 0xffff);
    set_imm(3 + p, x & 0xffff);
  } else {
    ShouldNotReachHere();
  }
}

AddressLiteral MacroAssembler::allocate_metadata_address(Metadata* obj) {
  assert(oop_recorder() != NULL, "this assembler needs a Recorder");
  int index = oop_recorder()->allocate_metadata_index(obj);
  RelocationHolder rspec = metadata_Relocation::spec(index);
  return AddressLiteral((address)obj, rspec);
}

AddressLiteral MacroAssembler::constant_metadata_address(Metadata* obj) {
  assert(oop_recorder() != NULL, "this assembler needs a Recorder");
  int index = oop_recorder()->find_index(obj);
  RelocationHolder rspec = metadata_Relocation::spec(index);
  return AddressLiteral((address)obj, rspec);
}

AddressLiteral MacroAssembler::allocate_oop_address(jobject obj) {
  assert(oop_recorder() != NULL, "this assembler needs an OopRecorder");
  int oop_index = oop_recorder()->allocate_oop_index(obj);
  return AddressLiteral(address(obj), oop_Relocation::spec(oop_index));
}

AddressLiteral MacroAssembler::constant_oop_address(jobject obj) {
  assert(oop_recorder() != NULL, "this assembler needs an OopRecorder");
  int oop_index = oop_recorder()->find_index(obj);
  return AddressLiteral(address(obj), oop_Relocation::spec(oop_index));
}

RegisterOrConstant MacroAssembler::delayed_value_impl(intptr_t* delayed_value_addr,
                                                      Register tmp, int offset) {
  intptr_t value = *delayed_value_addr;
  if (value != 0) {
    return RegisterOrConstant(value + offset);
  }

  // Load indirectly to solve generation ordering problem.
  // static address, no relocation
  int simm16_offset = load_const_optimized(tmp, delayed_value_addr, noreg, true);
  ld_PPC(tmp, simm16_offset, tmp); // must be aligned ((xa & 3) == 0)

  if (offset != 0) {
    addi_PPC(tmp, tmp, offset);
  }

  return RegisterOrConstant(tmp);
}

#ifndef PRODUCT
void MacroAssembler::pd_print_patched_instruction(address branch) {
  Unimplemented(); // TODO: RISCV port
}
#endif // ndef PRODUCT

// Conditional far branch for destinations encodable in 24+2 bits.
void MacroAssembler::bc_far(int boint, int biint, Label& dest, int optimize) {

  // If requested by flag optimize, relocate the bc_far as a
  // runtime_call and prepare for optimizing it when the code gets
  // relocated.
  if (optimize == bc_far_optimize_on_relocate) {
    relocate(relocInfo::runtime_call_type);
  }

  // variant 2:
  //
  //    b!cxx SKIP
  //    bxx   DEST
  //  SKIP:
  //

  const int opposite_boint = add_bhint_to_boint(opposite_bhint(inv_boint_bhint(boint)),
                                                opposite_bcond(inv_boint_bcond(boint)));

  // We emit two branches.
  // First, a conditional branch which jumps around the far branch.
  const address not_taken_pc = pc() + 2 * BytesPerInstWord;
  const address bc_pc        = pc();
  bc_PPC(opposite_boint, biint, not_taken_pc);

  const int bc_instr = *(int*)bc_pc;
  assert(not_taken_pc == (address)inv_bd_field(bc_instr, (intptr_t)bc_pc), "postcondition");
  assert(opposite_boint == inv_bo_field(bc_instr), "postcondition");
  assert(boint == add_bhint_to_boint(opposite_bhint(inv_boint_bhint(inv_bo_field(bc_instr))),
                                     opposite_bcond(inv_boint_bcond(inv_bo_field(bc_instr)))),
         "postcondition");
  assert(biint == inv_bi_field(bc_instr), "postcondition");

  // Second, an unconditional far branch which jumps to dest.
  // Note: target(dest) remembers the current pc (see CodeSection::target)
  //       and returns the current pc if the label is not bound yet; when
  //       the label gets bound, the unconditional far branch will be patched.
  const address target_pc = target(dest);
  const address b_pc  = pc();
  b_PPC(target_pc);

  assert(not_taken_pc == pc(),                     "postcondition");
  assert(dest.is_bound() || target_pc == b_pc, "postcondition");
}

// 1 or 2 instructions
void MacroAssembler::bc_far_optimized(int boint, int biint, Label& dest) {
  if (dest.is_bound() && is_within_range_of_bcxx(target(dest), pc())) {
    bc_PPC(boint, biint, dest);
  } else {
    bc_far(boint, biint, dest, MacroAssembler::bc_far_optimize_on_relocate);
  }
}

bool MacroAssembler::is_bc_far_at(address instruction_addr) {
  return is_bc_far_variant1_at(instruction_addr) ||
         is_bc_far_variant2_at(instruction_addr) ||
         is_bc_far_variant3_at(instruction_addr);
}

address MacroAssembler::get_dest_of_bc_far_at(address instruction_addr) {
  if (is_bc_far_variant1_at(instruction_addr)) {
    const address instruction_1_addr = instruction_addr;
    const int instruction_1 = *(int*)instruction_1_addr;
    return (address)inv_bd_field(instruction_1, (intptr_t)instruction_1_addr);
  } else if (is_bc_far_variant2_at(instruction_addr)) {
    const address instruction_2_addr = instruction_addr + 4;
    return bxx_destination_PPC(instruction_2_addr);
  } else if (is_bc_far_variant3_at(instruction_addr)) {
    return instruction_addr + 8;
  }
  // variant 4 ???
  ShouldNotReachHere();
  return NULL;
}
void MacroAssembler::set_dest_of_bc_far_at(address instruction_addr, address dest) {

  if (is_bc_far_variant3_at(instruction_addr)) {
    // variant 3, far cond branch to the next instruction, already patched to nops:
    //
    //    nop
    //    endgroup
    //  SKIP/DEST:
    //
    return;
  }

  // first, extract boint and biint from the current branch
  int boint = 0;
  int biint = 0;

  ResourceMark rm;
  const int code_size = 2 * BytesPerInstWord;
  CodeBuffer buf(instruction_addr, code_size);
  MacroAssembler masm(&buf);
  if (is_bc_far_variant2_at(instruction_addr) && dest == instruction_addr + 8) {
    // Far branch to next instruction: Optimize it by patching nops (produce variant 3).
    masm.nop_PPC();
    masm.endgroup_PPC();
  } else {
    if (is_bc_far_variant1_at(instruction_addr)) {
      // variant 1, the 1st instruction contains the destination address:
      //
      //    bcxx  DEST
      //    nop
      //
      const int instruction_1 = *(int*)(instruction_addr);
      boint = inv_bo_field(instruction_1);
      biint = inv_bi_field(instruction_1);
    } else if (is_bc_far_variant2_at(instruction_addr)) {
      // variant 2, the 2nd instruction contains the destination address:
      //
      //    b!cxx SKIP
      //    bxx   DEST
      //  SKIP:
      //
      const int instruction_1 = *(int*)(instruction_addr);
      boint = add_bhint_to_boint(opposite_bhint(inv_boint_bhint(inv_bo_field(instruction_1))),
          opposite_bcond(inv_boint_bcond(inv_bo_field(instruction_1))));
      biint = inv_bi_field(instruction_1);
    } else {
      // variant 4???
      ShouldNotReachHere();
    }

    // second, set the new branch destination and optimize the code
    if (dest != instruction_addr + 4 && // the bc_far is still unbound!
        masm.is_within_range_of_bcxx(dest, instruction_addr)) {
      // variant 1:
      //
      //    bcxx  DEST
      //    nop
      //
      masm.bc_PPC(boint, biint, dest);
      masm.nop_PPC();
    } else {
      // variant 2:
      //
      //    b!cxx SKIP
      //    bxx   DEST
      //  SKIP:
      //
      const int opposite_boint = add_bhint_to_boint(opposite_bhint(inv_boint_bhint(boint)),
                                                    opposite_bcond(inv_boint_bcond(boint)));
      const address not_taken_pc = masm.pc() + 2 * BytesPerInstWord;
      masm.bc_PPC(opposite_boint, biint, not_taken_pc);
      masm.b_PPC(dest);
    }
  }
  ICache::riscv64_flush_icache_bytes(instruction_addr, code_size);
}

// Emit a NOT mt-safe patchable 64 bit absolute call/jump.
void MacroAssembler::bxx64_patchable(address dest, relocInfo::relocType rt, bool link) {
  // get current pc
  uint64_t start_pc = (uint64_t) pc();

  const address pc_of_bl = (address) (start_pc + (6*BytesPerInstWord)); // bl is last
  const address pc_of_b  = (address) (start_pc + (0*BytesPerInstWord)); // b is first

  // relocate here
  if (rt != relocInfo::none) {
    relocate(rt);
  }

  if ( ReoptimizeCallSequences &&
       (( link && is_within_range_of_b(dest, pc_of_bl)) ||
        (!link && is_within_range_of_b(dest, pc_of_b)))) {
    // variant 2:
    // Emit an optimized, pc-relative call/jump.

    if (link) {
      // some padding
      nop_PPC();
      nop_PPC();
      nop_PPC();
      nop_PPC();
      nop_PPC();
      nop_PPC();

      // do the call
      assert(pc() == pc_of_bl, "just checking");
      bl_PPC(dest, relocInfo::none);
    } else {
      // do the jump
      assert(pc() == pc_of_b, "just checking");
      b_PPC(dest, relocInfo::none);

      // some padding
      nop_PPC();
      nop_PPC();
      nop_PPC();
      nop_PPC();
      nop_PPC();
      nop_PPC();
    }

    // Assert that we can identify the emitted call/jump.
    assert(is_bxx64_patchable_variant2_at((address)start_pc, link),
           "can't identify emitted call");
  } else {
    // variant 1:
    mr_PPC(R0, R11);  // spill R11 -> R0.

    // Load the destination address into CTR,
    // calculate destination relative to global toc.
    calculate_address_from_global_toc(R11, dest, true, true, false);

    mtctr_PPC(R11);
    mr_PPC(R11, R0);  // spill R11 <- R0.
    nop_PPC();

    // do the call/jump
    if (link) {
      bctrl_PPC();
    } else{
      bctr_PPC();
    }
    // Assert that we can identify the emitted call/jump.
    assert(is_bxx64_patchable_variant1b_at((address)start_pc, link),
           "can't identify emitted call");
  }

  // Assert that we can identify the emitted call/jump.
  assert(is_bxx64_patchable_at((address)start_pc, link),
         "can't identify emitted call");
  assert(get_dest_of_bxx64_patchable_at((address)start_pc, link) == dest,
         "wrong encoding of dest address");
}

// Identify a bxx64_patchable instruction.
bool MacroAssembler::is_bxx64_patchable_at(address instruction_addr, bool link) {
  return is_bxx64_patchable_variant1b_at(instruction_addr, link)
    //|| is_bxx64_patchable_variant1_at(instruction_addr, link)
      || is_bxx64_patchable_variant2_at(instruction_addr, link);
}

// Does the call64_patchable instruction use a pc-relative encoding of
// the call destination?
bool MacroAssembler::is_bxx64_patchable_pcrelative_at(address instruction_addr, bool link) {
  // variant 2 is pc-relative
  return is_bxx64_patchable_variant2_at(instruction_addr, link);
}

// Identify variant 1.
bool MacroAssembler::is_bxx64_patchable_variant1_at(address instruction_addr, bool link) {
  unsigned int* instr = (unsigned int*) instruction_addr;
  return (link ? is_bctrl(instr[6]) : is_bctr(instr[6])) // bctr[l]
      && is_mtctr(instr[5]) // mtctr
    && is_load_const_at(instruction_addr);
}

// Identify variant 1b: load destination relative to global toc.
bool MacroAssembler::is_bxx64_patchable_variant1b_at(address instruction_addr, bool link) {
  unsigned int* instr = (unsigned int*) instruction_addr;
  return (link ? is_bctrl(instr[6]) : is_bctr(instr[6])) // bctr[l]
    && is_mtctr(instr[3]) // mtctr
    && is_calculate_address_from_global_toc_at(instruction_addr + 2*BytesPerInstWord, instruction_addr);
}

// Identify variant 2.
bool MacroAssembler::is_bxx64_patchable_variant2_at(address instruction_addr, bool link) {
  unsigned int* instr = (unsigned int*) instruction_addr;
  if (link) {
    return is_bl (instr[6])  // bl dest is last
      && is_nop(instr[0])  // nop
      && is_nop(instr[1])  // nop
      && is_nop(instr[2])  // nop
      && is_nop(instr[3])  // nop
      && is_nop(instr[4])  // nop
      && is_nop(instr[5]); // nop
  } else {
    return is_b  (instr[0])  // b  dest is first
      && is_nop(instr[1])  // nop
      && is_nop(instr[2])  // nop
      && is_nop(instr[3])  // nop
      && is_nop(instr[4])  // nop
      && is_nop(instr[5])  // nop
      && is_nop(instr[6]); // nop
  }
}

// Set dest address of a bxx64_patchable instruction.
void MacroAssembler::set_dest_of_bxx64_patchable_at(address instruction_addr, address dest, bool link) {
  ResourceMark rm;
  int code_size = MacroAssembler::bxx64_patchable_size;
  CodeBuffer buf(instruction_addr, code_size);
  MacroAssembler masm(&buf);
  masm.bxx64_patchable(dest, relocInfo::none, link);
  ICache::riscv64_flush_icache_bytes(instruction_addr, code_size);
}

// Get dest address of a bxx64_patchable instruction.
address MacroAssembler::get_dest_of_bxx64_patchable_at(address instruction_addr, bool link) {
  if (is_bxx64_patchable_variant1_at(instruction_addr, link)) {
    return (address) (unsigned long) get_const(instruction_addr);
  } else if (is_bxx64_patchable_variant2_at(instruction_addr, link)) {
    unsigned int* instr = (unsigned int*) instruction_addr;
    if (link) {
      const int instr_idx = 6; // bl is last
      int branchoffset = branch_destination(instr[instr_idx], 0);
      return instruction_addr + branchoffset + instr_idx*BytesPerInstWord;
    } else {
      const int instr_idx = 0; // b is first
      int branchoffset = branch_destination(instr[instr_idx], 0);
      return instruction_addr + branchoffset + instr_idx*BytesPerInstWord;
    }
  // Load dest relative to global toc.
  } else if (is_bxx64_patchable_variant1b_at(instruction_addr, link)) {
    return get_address_of_calculate_address_from_global_toc_at(instruction_addr + 2*BytesPerInstWord,
                                                               instruction_addr);
  } else {
    ShouldNotReachHere();
    return NULL;
  }
}

void MacroAssembler::save_abi_frame(Register dst, int offset) {
  offset -= 8;  sd(R1_RA,  dst, offset);
  offset -= 8;  sd(R8_FP,  dst, offset);
}

void MacroAssembler::restore_abi_frame(Register dst, int offset) {
  offset -= 8;  ld(R1_RA,  dst, offset);
  offset -= 8;  ld(R8_FP,  dst, offset);
}

void MacroAssembler::save_nonvolatile_gprs(Register dst, int offset) {
  offset -= 8;  sd(R2,  dst, offset);
  offset -= 8;  sd(R9,  dst, offset);
  offset -= 8;  sd(R18, dst, offset);
  offset -= 8;  sd(R19, dst, offset);
  offset -= 8;  sd(R20, dst, offset);
  offset -= 8;  sd(R21, dst, offset);
  offset -= 8;  sd(R22, dst, offset);
  offset -= 8;  sd(R23, dst, offset);
  offset -= 8;  sd(R24, dst, offset);
  offset -= 8;  sd(R25, dst, offset);
  offset -= 8;  sd(R26, dst, offset);
  offset -= 8;  sd(R27, dst, offset);

  offset -= 8;  fsd(F8,  dst, offset);
  offset -= 8;  fsd(F9,  dst, offset);
  offset -= 8;  fsd(F18, dst, offset);
  offset -= 8;  fsd(F19, dst, offset);
  offset -= 8;  fsd(F20, dst, offset);
  offset -= 8;  fsd(F21, dst, offset);
  offset -= 8;  fsd(F22, dst, offset);
  offset -= 8;  fsd(F23, dst, offset);
  offset -= 8;  fsd(F24, dst, offset);
  offset -= 8;  fsd(F25, dst, offset);
  offset -= 8;  fsd(F26, dst, offset);
  offset -= 8;  fsd(F27, dst, offset);
}

void MacroAssembler::restore_nonvolatile_gprs(Register src, int offset) {
  offset -= 8;  ld(R2,  src, offset);
  offset -= 8;  ld(R9,  src, offset);
  offset -= 8;  ld(R18, src, offset);
  offset -= 8;  ld(R19, src, offset);
  offset -= 8;  ld(R20, src, offset);
  offset -= 8;  ld(R21, src, offset);
  offset -= 8;  ld(R22, src, offset);
  offset -= 8;  ld(R23, src, offset);
  offset -= 8;  ld(R24, src, offset);
  offset -= 8;  ld(R25, src, offset);
  offset -= 8;  ld(R26, src, offset);
  offset -= 8;  ld(R27, src, offset);

  offset -= 8;  fld(F8,  src, offset);
  offset -= 8;  fld(F9,  src, offset);
  offset -= 8;  fld(F18, src, offset);
  offset -= 8;  fld(F19, src, offset);
  offset -= 8;  fld(F20, src, offset);
  offset -= 8;  fld(F21, src, offset);
  offset -= 8;  fld(F22, src, offset);
  offset -= 8;  fld(F23, src, offset);
  offset -= 8;  fld(F24, src, offset);
  offset -= 8;  fld(F25, src, offset);
  offset -= 8;  fld(F26, src, offset);
  offset -= 8;  fld(F27, src, offset);
}

// For verify_oops.
void MacroAssembler::save_volatile_gprs(Register dst, int offset) {
  offset -= 8;  sd(R1,  dst, offset);
  offset -= 8;  sd(R5,  dst, offset);
  offset -= 8;  sd(R6,  dst, offset);
  offset -= 8;  sd(R7,  dst, offset);
  offset -= 8;  sd(R10, dst, offset);
  offset -= 8;  sd(R11, dst, offset);
  offset -= 8;  sd(R12, dst, offset);
  offset -= 8;  sd(R13, dst, offset);
  offset -= 8;  sd(R14, dst, offset);
  offset -= 8;  sd(R15, dst, offset);
  offset -= 8;  sd(R16, dst, offset);
  offset -= 8;  sd(R17, dst, offset);
  offset -= 8;  sd(R28, dst, offset);
  offset -= 8;  sd(R29, dst, offset);
  offset -= 8;  sd(R30, dst, offset);
  offset -= 8;  sd(R31, dst, offset);

  offset -= 8;  fsd(F0,  dst, offset);
  offset -= 8;  fsd(F1,  dst, offset);
  offset -= 8;  fsd(F2,  dst, offset);
  offset -= 8;  fsd(F3,  dst, offset);
  offset -= 8;  fsd(F4,  dst, offset);
  offset -= 8;  fsd(F5,  dst, offset);
  offset -= 8;  fsd(F6,  dst, offset);
  offset -= 8;  fsd(F7,  dst, offset);
  offset -= 8;  fsd(F10, dst, offset);
  offset -= 8;  fsd(F11, dst, offset);
  offset -= 8;  fsd(F12, dst, offset);
  offset -= 8;  fsd(F13, dst, offset);
  offset -= 8;  fsd(F14, dst, offset);
  offset -= 8;  fsd(F15, dst, offset);
  offset -= 8;  fsd(F16, dst, offset);
  offset -= 8;  fsd(F17, dst, offset);
  offset -= 8;  fsd(F28, dst, offset);
  offset -= 8;  fsd(F29, dst, offset);
  offset -= 8;  fsd(F30, dst, offset);
  offset -= 8;  fsd(F31, dst, offset);
}

// For verify_oops.
void MacroAssembler::restore_volatile_gprs(Register src, int offset) {
  offset -= 8;  ld_PPC(R2,  offset, src);
  offset -= 8;  ld_PPC(R3,  offset, src);
  offset -= 8;  ld_PPC(R4,  offset, src);
  offset -= 8;  ld_PPC(R5,  offset, src);
  offset -= 8;  ld_PPC(R6,  offset, src);
  offset -= 8;  ld_PPC(R7,  offset, src);
  offset -= 8;  ld_PPC(R8,  offset, src);
  offset -= 8;  ld_PPC(R9,  offset, src);
  offset -= 8;  ld_PPC(R10, offset, src);
  offset -= 8;  ld_PPC(R11, offset, src);
  offset -= 8;  ld_PPC(R12, offset, src);

  offset -= 8;  lfd_PPC(F0,  offset, src);
  offset -= 8;  lfd_PPC(F1,  offset, src);
  offset -= 8;  lfd_PPC(F2,  offset, src);
  offset -= 8;  lfd_PPC(F3,  offset, src);
  offset -= 8;  lfd_PPC(F4,  offset, src);
  offset -= 8;  lfd_PPC(F5,  offset, src);
  offset -= 8;  lfd_PPC(F6,  offset, src);
  offset -= 8;  lfd_PPC(F7,  offset, src);
  offset -= 8;  lfd_PPC(F8,  offset, src);
  offset -= 8;  lfd_PPC(F9,  offset, src);
  offset -= 8;  lfd_PPC(F10, offset, src);
  offset -= 8;  lfd_PPC(F11, offset, src);
  offset -= 8;  lfd_PPC(F12, offset, src);
  offset -= 8;  lfd_PPC(F13, offset, src);
}

void MacroAssembler::save_LR_CR(Register tmp) {
  mfcr_PPC(tmp);
  std_PPC(tmp, _abi_PPC(cr), R1_SP_PPC);
  mflr_PPC(tmp);
  std_PPC(tmp, _abi_PPC(lr), R1_SP_PPC);
  // Tmp must contain lr on exit! (see return_addr and prolog in riscv64.ad)
}

void MacroAssembler::restore_LR_CR(Register tmp) {
  assert(tmp != R1_SP_PPC, "must be distinct");
  ld_PPC(tmp, _abi_PPC(lr), R1_SP_PPC);
  mtlr_PPC(tmp);
  ld_PPC(tmp, _abi_PPC(cr), R1_SP_PPC);
  mtcr_PPC(tmp);
}

address MacroAssembler::get_PC_trash_LR(Register result) {
  Label L;
  bl_PPC(L);
  bind(L);
  address lr_pc = pc();
  mflr_PPC(result);
  return lr_pc;
}

void MacroAssembler::resize_frame(Register offset, Register tmp) {
#ifdef ASSERT
  assert_different_registers(offset, tmp, R2_SP);
  andi(tmp, offset, frame::alignment_in_bytes-1);
  asm_assert_eq(tmp, R0_ZERO, "resize_frame: unaligned", 0x204);
#endif
  add(R2_SP, R2_SP, offset);
}

void MacroAssembler::resize_frame(int offset, Register tmp) {
  assert(is_simm(offset, 12), "too big an offset");
  assert_different_registers(tmp, R2_SP);
  assert((offset & (frame::alignment_in_bytes-1))==0, "resize_frame: unaligned");
  addi(R2_SP, R2_SP, offset);
}

void MacroAssembler::resize_frame_absolute(Register addr, Register tmp) {
  #ifdef ASSERT
    assert_different_registers(addr, tmp, R2_SP);
    andi(tmp, addr, frame::alignment_in_bytes-1);
    asm_assert_eq(tmp, R0_ZERO, "resize_frame: unaligned", 0x204);
  #endif
    mv(R2_SP, addr);
}

void MacroAssembler::push_frame(Register bytes, Register tmp) {
#ifdef ASSERT
  andi(tmp, bytes, frame::alignment_in_bytes-1);
  asm_assert_eq(tmp, R0_ZERO, "push_frame(Reg, Reg): unaligned", 0x203);
#endif
  mv(R8_FP, R2_SP);
  sub(R2_SP, R2_SP, bytes);
}

// Push a frame of size `bytes'.
void MacroAssembler::push_frame(unsigned int bytes, Register tmp) {
  long offset = align_addr(bytes, frame::alignment_in_bytes);
  mv(R8_FP, R2_SP);
  if (is_simm(-offset, 12)) {
    addi(R2_SP, R2_SP, -offset);
  } else {
    li(tmp, -offset);
    sub(R2_SP, R2_SP, tmp);
  }
}

// Push a frame of size `bytes' plus abi_reg_args_ppc on top.
void MacroAssembler::push_frame_reg_args(unsigned int bytes, Register tmp) {
  should_not_reach_here(); // PPC abi
  push_frame(bytes + frame::abi_reg_args_ppc_size, tmp);
}

// Pop current C frame.
void MacroAssembler::pop_C_frame(bool restoreRA) {
  mv(R2_SP, R8_FP);
  if (restoreRA) {
    ld(R1_RA, R8_FP, _abi(ra));
  }
  ld(R8_FP, R8_FP, _abi(fp));
}

void MacroAssembler::pop_java_frame(bool restoreRA) {
//  ld(R21_sender_SP, R8_FP, _ijava_state(sender_sp)); TODO RISCV we don't need to restore register here
  if (restoreRA) {
    ld(R1_RA, R8_FP, _abi(ra));
  }
  ld(R8_FP, R8_FP, _abi(fp));
  mv(R2_SP, R21_sender_SP);
#ifdef ASSERT
  {
//    Label Lok;
    // TODO_RISCV take registers for assert
//    ld(Rscratch1, R8_FP, _ijava_state(ijava_reserved));
//    li(Rscratch2, 0x5afe);
//    beq(Rscratch1, Rscratch2, Lok);
//    stop("frame corrupted (remove activation)", 0x5afe);
//    bind(Lok);
  }
#endif
}

address MacroAssembler::branch_to(Register r_function_entry, bool and_link) {
  if (and_link) {
    jalr(r_function_entry);
  } else {
    jr(r_function_entry);
  }
  _last_calls_return_pc = pc();

  return _last_calls_return_pc;
}

// Call a C function via a function descriptor and use full C
// calling conventions. Updates and returns _last_calls_return_pc.
address MacroAssembler::call_c(Register r_function_entry) {
  return branch_to(r_function_entry, /*and_link=*/true);
}

// For tail calls: only branch, don't link, so callee returns to caller of this function.
address MacroAssembler::call_c_and_return_to_caller(Register r_function_entry) {
  return branch_to(r_function_entry, /*and_link=*/false);
}

address MacroAssembler::call_c(address function_entry, relocInfo::relocType rt) {
  li(R6_scratch2, function_entry);
  return branch_to(R6_scratch2,  /*and_link=*/true);
}

void MacroAssembler::call_VM_base(Register oop_result,
                                  Register last_java_sp,
                                  address  entry_point,
                                  bool     check_exceptions) {
  BLOCK_COMMENT("call_VM {");
  // Determine last_java_sp register.
  if (!last_java_sp->is_valid()) {
    last_java_sp = R2_SP;
  }
  set_top_ijava_frame_at_SP_as_last_Java_frame(last_java_sp, R8_FP, R5_scratch1);

  // ARG0 must hold thread address.
  mv(R10_ARG0, R24_thread);
  address return_pc = call_c(entry_point, relocInfo::none);

  reset_last_Java_frame();

  // Check for pending exceptions.
  if (check_exceptions) {
    // We don't check for exceptions here.
    ShouldNotReachHere();
  }

  // Get oop result if there is one and reset the value in the thread.
  if (oop_result->is_valid()) {
    get_vm_result(oop_result);
  }

  _last_calls_return_pc = return_pc;
  BLOCK_COMMENT("} call_VM");
}

void MacroAssembler::call_VM_leaf_base(address entry_point) {
  BLOCK_COMMENT("call_VM_leaf {");
  call_c(entry_point, relocInfo::none);
  BLOCK_COMMENT("} call_VM_leaf");
}

void MacroAssembler::call_VM(Register oop_result, address entry_point, bool check_exceptions) {
  call_VM_base(oop_result, noreg, entry_point, check_exceptions);
}

void MacroAssembler::call_VM(Register oop_result, address entry_point, Register arg_1,
                             bool check_exceptions) {
  // R10_ARG0 is reserved for the thread.
  mv_if_needed(R11_ARG1, arg_1);
  call_VM(oop_result, entry_point, check_exceptions);
}

void MacroAssembler::call_VM(Register oop_result, address entry_point, Register arg_1, Register arg_2,
                             bool check_exceptions) {
  // R10_ARG0 is reserved for the thread
  mv_if_needed(R11_ARG1, arg_1);
  assert(arg_2 != R11_ARG1, "smashed argument");
  mv_if_needed(R12_ARG2, arg_2);
  call_VM(oop_result, entry_point, check_exceptions);
}

void MacroAssembler::call_VM(Register oop_result, address entry_point, Register arg_1, Register arg_2, Register arg_3,
                             bool check_exceptions) {
  // R10_ARG0 is reserved for the thread
  mv_if_needed(R11_ARG1, arg_1);
  assert(arg_2 != R11_ARG1, "smashed argument");
  mv_if_needed(R12_ARG2, arg_2);
  mv_if_needed(R13_ARG3, arg_3);
  call_VM(oop_result, entry_point, check_exceptions);
}

void MacroAssembler::call_VM_leaf(address entry_point) {
  call_VM_leaf_base(entry_point);
}

void MacroAssembler::call_VM_leaf(address entry_point, Register arg_0) {
  mv_if_needed(R10_ARG0, arg_0);
  call_VM_leaf(entry_point);
}

void MacroAssembler::call_VM_leaf(address entry_point, Register arg_0, Register arg_1) {
  mv_if_needed(R10_ARG0, arg_0);
  assert(arg_1 != R10_ARG0, "smashed argument");
  mv_if_needed(R11_ARG1, arg_1);
  call_VM_leaf(entry_point);
}

void MacroAssembler::call_VM_leaf(address entry_point, Register arg_0, Register arg_1, Register arg_2) {
  mv_if_needed(R10_ARG0, arg_0);
  assert(arg_1 != R10_ARG0, "smashed argument");
  mv_if_needed(R11_ARG1, arg_1);
  assert(arg_2 != R11_ARG1 && arg_2 != R10_ARG0, "smashed argument");
  mv_if_needed(R12_ARG2, arg_2);
  call_VM_leaf(entry_point);
}

// Check whether instruction is a read access to the polling page
// which was emitted by load_from_polling_page(..).
bool MacroAssembler::is_load_from_polling_page(int instruction, void* ucontext,
                                               address* polling_address_ptr) {
  if (!is_ld(instruction))
    return false; // It's not a ld. Fail.

  int rt = inv_rt_field(instruction);
  int ra = inv_ra_field(instruction);
  int ds = inv_ds_field(instruction);
  if (!(ds == 0 && ra != 0 && rt == 0)) {
    return false; // It's not a ld_PPC(r0, X, ra). Fail.
  }

  if (!ucontext) {
    // Set polling address.
    if (polling_address_ptr != NULL) {
      *polling_address_ptr = NULL;
    }
    return true; // No ucontext given. Can't check value of ra. Assume true.
  }

#ifdef LINUX
  // Ucontext given. Check that register ra contains the address of
  // the safepoing polling page.
  ucontext_t* uc = (ucontext_t*) ucontext;
  // Set polling address.
/* // FIXME_RISCV begin
  address addr = (address)uc->uc_mcontext.regs->gpr[ra] + (ssize_t)ds;
  if (polling_address_ptr != NULL) {
    *polling_address_ptr = addr;
  }
 */
  return false;//os::is_poll_address(addr);
// FIXME_RISCV end
#else
  // Not on Linux, ucontext must be NULL.
  ShouldNotReachHere();
  return false;
#endif
}

void MacroAssembler::bang_stack_with_offset(int offset) {
  // When increasing the stack, the old stack pointer will be written
  // to the new top of stack according to the RISCV64 abi.
  // Therefore, stack banging is not necessary when increasing
  // the stack by <= os::vm_page_size() bytes.
  // When increasing the stack by a larger amount, this method is
  // called repeatedly to bang the intermediate pages.

  // Stack grows down, caller passes positive offset.
  assert(offset > 0, "must bang with positive offset");

  long stdoffset = -offset;

  if (is_simm(stdoffset, 12)) {
    // Signed 12 bit offset, a simple sd is ok.
    if (UseLoadInstructionsForStackBangingRISCV64) {
      ld(R0, R2_SP, (int)(signed short)stdoffset);
    } else {
      sd(R0, R2_SP, (int)(signed short)stdoffset);
    }
  } else if (is_simm(stdoffset, 31)) {
    Register tmp = R10_ARG0;
    const int lo = add_const_optimized(tmp, R2_SP, stdoffset, noreg, true);
    if (UseLoadInstructionsForStackBangingRISCV64) {
      ld(R0, tmp, lo);
    } else {
      sd(R0, tmp, lo);
    }
  } else {
    ShouldNotReachHere();
  }
}

// If instruction is a stack bang of the form
//    std    R0,    x(Ry),       (see bang_stack_with_offset())
//    stdu   R1_SP_PPC, x(R1_SP_PPC),    (see push_frame(), resize_frame())
// or stdux  R1_SP_PPC, Rx, R1_SP_PPC    (see push_frame(), resize_frame())
// return the banged address. Otherwise, return 0.
address MacroAssembler::get_stack_bang_address(int instruction, void *ucontext) {
#ifdef LINUX
//  ucontext_t* uc = (ucontext_t*) ucontext;
//  int rs = inv_rs_field(instruction);
//  int ra = inv_ra_field(instruction);
//  if (   (is_ld(instruction)   && rs == 0 &&  UseLoadInstructionsForStackBangingRISCV64)
//      || (is_std(instruction)  && rs == 0 && !UseLoadInstructionsForStackBangingRISCV64)
//      || (is_stdu(instruction) && rs == 1)) {
//    int ds = inv_ds_field(instruction);
//    // return banged address
//    return ds+(address)uc->uc_mcontext.regs->gpr[ra];
//  } else if (is_stdux(instruction) && rs == 1) {
//    int rb = inv_rb_field(instruction);
//    address sp = (address)uc->uc_mcontext.regs->gpr[1];
//    long rb_val = (long)uc->uc_mcontext.regs->gpr[rb];
//    return ra != 1 || rb_val >= 0 ? NULL         // not a stack bang
//                                  : sp + rb_val; // banged address
//  }
  return NULL; // not a stack bang
#else
  // workaround not needed on !LINUX :-)
  ShouldNotCallThis();
  return NULL;
#endif
}

void MacroAssembler::reserved_stack_check(Register return_pc) {
  // Test if reserved zone needs to be enabled.
  Label no_reserved_zone_enabling;

  ld_ptr_PPC(R0, JavaThread::reserved_stack_activation_offset(), R24_thread);
  cmpld_PPC(CCR0, R1_SP_PPC, R0);
  blt_predict_taken_PPC(CCR0, no_reserved_zone_enabling);

  // Enable reserved zone again, throw stack overflow exception.
  push_frame_reg_args(0, R0);
  call_VM_leaf(CAST_FROM_FN_PTR(address, SharedRuntime::enable_stack_reserved_zone), R24_thread);
  pop_C_frame(false);
  mtlr_PPC(return_pc);
  load_const_optimized(R0, StubRoutines::throw_delayed_StackOverflowError_entry());
  mtctr_PPC(R0);
  bctr_PPC();

  should_not_reach_here();

  bind(no_reserved_zone_enabling);
}

void MacroAssembler::getandsetd(Register dest_current_value, Register exchange_value, Register addr_base,
                                bool cmpxchgx_hint) {
  Label retry;
  bind(retry);
  ldarx_PPC(dest_current_value, addr_base, cmpxchgx_hint);
  stdcx__PPC(exchange_value, addr_base);
  if (UseStaticBranchPredictionInCompareAndSwapRISCV64) {
    bne_predict_not_taken_PPC(CCR0, retry); // StXcx_ sets CCR0.
  } else {
    bne_PPC(                  CCR0, retry); // StXcx_ sets CCR0.
  }
}

void MacroAssembler::getandaddd(Register dest_current_value, Register inc_value, Register addr_base,
                                Register tmp, bool cmpxchgx_hint) {
  Label retry;
  bind(retry);
  ldarx_PPC(dest_current_value, addr_base, cmpxchgx_hint);
  add_PPC(tmp, dest_current_value, inc_value);
  stdcx__PPC(tmp, addr_base);
  if (UseStaticBranchPredictionInCompareAndSwapRISCV64) {
    bne_predict_not_taken_PPC(CCR0, retry); // StXcx_ sets CCR0.
  } else {
    bne_PPC(                  CCR0, retry); // StXcx_ sets CCR0.
  }
}

// Word/sub-word atomic helper functions

// Temps and addr_base are killed if size < 4 and processor does not support respective instructions.
// Only signed types are supported with size < 4.
// Atomic add always kills tmp1.
void MacroAssembler::atomic_get_and_modify_generic(Register dest_current_value, Register exchange_value,
                                                   Register addr_base, Register tmp1, Register tmp2, Register tmp3,
                                                   bool cmpxchgx_hint, bool is_add, int size) {
  // Sub-word instructions are available since Power 8.
  // For older processors, instruction_type != size holds, and we
  // emulate the sub-word instructions by constructing a 4-byte value
  // that leaves the other bytes unchanged.
  const int instruction_type = VM_Version::has_lqarx() ? size : 4;

  Label retry;
  Register shift_amount = noreg,
           val32 = dest_current_value,
           modval = is_add ? tmp1 : exchange_value;

  if (instruction_type != size) {
    assert_different_registers(tmp1, tmp2, tmp3, dest_current_value, exchange_value, addr_base);
    modval = tmp1;
    shift_amount = tmp2;
    val32 = tmp3;
    // Need some preperation: Compute shift amount, align address. Note: shorts must be 2 byte aligned.
#ifdef VM_LITTLE_ENDIAN
    rldic_PPC(shift_amount, addr_base, 3, 64-5); // (dest & 3) * 8;
    clrrdi_PPC(addr_base, addr_base, 2);
#else
    xori_PPC(shift_amount, addr_base, (size == 1) ? 3 : 2);
    clrrdi_PPC(addr_base, addr_base, 2);
    rldic_PPC(shift_amount, shift_amount, 3, 64-5); // byte: ((3-dest) & 3) * 8; short: ((1-dest/2) & 1) * 16;
#endif
  }

  // atomic emulation loop
  bind(retry);

  switch (instruction_type) {
    case 4: lwarx_PPC(val32, addr_base, cmpxchgx_hint); break;
    case 2: lharx_PPC(val32, addr_base, cmpxchgx_hint); break;
    case 1: lbarx_PPC(val32, addr_base, cmpxchgx_hint); break;
    default: ShouldNotReachHere();
  }

  if (instruction_type != size) {
    srw_PPC(dest_current_value, val32, shift_amount);
  }

  if (is_add) { add_PPC(modval, dest_current_value, exchange_value); }

  if (instruction_type != size) {
    // Transform exchange value such that the replacement can be done by one xor instruction.
    xorr_PPC(modval, dest_current_value, is_add ? modval : exchange_value);
    clrldi_PPC(modval, modval, (size == 1) ? 56 : 48);
    slw_PPC(modval, modval, shift_amount);
    xorr_PPC(modval, val32, modval);
  }

  switch (instruction_type) {
    case 4: stwcx__PPC(modval, addr_base); break;
    case 2: sthcx__PPC(modval, addr_base); break;
    case 1: stbcx__PPC(modval, addr_base); break;
    default: ShouldNotReachHere();
  }

  if (UseStaticBranchPredictionInCompareAndSwapRISCV64) {
    bne_predict_not_taken_PPC(CCR0, retry); // StXcx_ sets CCR0.
  } else {
    bne_PPC(                  CCR0, retry); // StXcx_ sets CCR0.
  }

  // l?arx zero-extends, but Java wants byte/short values sign-extended.
  if (size == 1) {
    extsb_PPC(dest_current_value, dest_current_value);
  } else if (size == 2) {
    extsh_PPC(dest_current_value, dest_current_value);
  };
}

// Temps, addr_base and exchange_value are killed if size < 4 and processor does not support respective instructions.
// Only signed types are supported with size < 4.
void MacroAssembler::cmpxchg_loop_body(ConditionRegister flag, Register dest_current_value,
                                       Register compare_value, Register exchange_value,
                                       Register addr_base, Register tmp1, Register tmp2,
                                       Label &retry, Label &failed, bool cmpxchgx_hint, int size) {
  // Sub-word instructions are available since Power 8.
  // For older processors, instruction_type != size holds, and we
  // emulate the sub-word instructions by constructing a 4-byte value
  // that leaves the other bytes unchanged.
  const int instruction_type = VM_Version::has_lqarx() ? size : 4;

  Register shift_amount = noreg,
           val32 = dest_current_value,
           modval = exchange_value;

  if (instruction_type != size) {
    assert_different_registers(tmp1, tmp2, dest_current_value, compare_value, exchange_value, addr_base);
    shift_amount = tmp1;
    val32 = tmp2;
    modval = tmp2;
    // Need some preperation: Compute shift amount, align address. Note: shorts must be 2 byte aligned.
#ifdef VM_LITTLE_ENDIAN
    rldic_PPC(shift_amount, addr_base, 3, 64-5); // (dest & 3) * 8;
    clrrdi_PPC(addr_base, addr_base, 2);
#else
    xori_PPC(shift_amount, addr_base, (size == 1) ? 3 : 2);
    clrrdi_PPC(addr_base, addr_base, 2);
    rldic_PPC(shift_amount, shift_amount, 3, 64-5); // byte: ((3-dest) & 3) * 8; short: ((1-dest/2) & 1) * 16;
#endif
    // Transform exchange value such that the replacement can be done by one xor instruction.
    xorr_PPC(exchange_value, compare_value, exchange_value);
    clrldi_PPC(exchange_value, exchange_value, (size == 1) ? 56 : 48);
    slw_PPC(exchange_value, exchange_value, shift_amount);
  }

  // atomic emulation loop
  bind(retry);

  switch (instruction_type) {
    case 4: lwarx_PPC(val32, addr_base, cmpxchgx_hint); break;
    case 2: lharx_PPC(val32, addr_base, cmpxchgx_hint); break;
    case 1: lbarx_PPC(val32, addr_base, cmpxchgx_hint); break;
    default: ShouldNotReachHere();
  }

  if (instruction_type != size) {
    srw_PPC(dest_current_value, val32, shift_amount);
  }
  if (size == 1) {
    extsb_PPC(dest_current_value, dest_current_value);
  } else if (size == 2) {
    extsh_PPC(dest_current_value, dest_current_value);
  };

  cmpw_PPC(flag, dest_current_value, compare_value);
  if (UseStaticBranchPredictionInCompareAndSwapRISCV64) {
    bne_predict_not_taken_PPC(flag, failed);
  } else {
    bne_PPC(                  flag, failed);
  }
  // branch to done  => (flag == ne), (dest_current_value != compare_value)
  // fall through    => (flag == eq), (dest_current_value == compare_value)

  if (instruction_type != size) {
    xorr_PPC(modval, val32, exchange_value);
  }

  switch (instruction_type) {
    case 4: stwcx__PPC(modval, addr_base); break;
    case 2: sthcx__PPC(modval, addr_base); break;
    case 1: stbcx__PPC(modval, addr_base); break;
    default: ShouldNotReachHere();
  }
}

// CmpxchgX sets condition register to cmpX(current, compare).
void MacroAssembler::cmpxchg_generic(ConditionRegister flag, Register dest_current_value,
                                     Register compare_value, Register exchange_value,
                                     Register addr_base, Register tmp1, Register tmp2,
                                     int semantics, bool cmpxchgx_hint,
                                     Register int_flag_success, bool contention_hint, bool weak, int size) {
  Label retry;
  Label failed;
  Label done;

  // Save one branch if result is returned via register and
  // result register is different from the other ones.
  bool use_result_reg    = (int_flag_success != noreg);
  bool preset_result_reg = (int_flag_success != dest_current_value && int_flag_success != compare_value &&
                            int_flag_success != exchange_value && int_flag_success != addr_base &&
                            int_flag_success != tmp1 && int_flag_success != tmp2);
  assert(!weak || flag == CCR0, "weak only supported with CCR0");
  assert(size == 1 || size == 2 || size == 4, "unsupported");

  if (use_result_reg && preset_result_reg) {
    li_PPC(int_flag_success, 0); // preset (assume cas failed)
  }

  // Add simple guard in order to reduce risk of starving under high contention (recommended by IBM).
  if (contention_hint) { // Don't try to reserve if cmp fails.
    switch (size) {
      case 1: lbz_PPC(dest_current_value, 0, addr_base); extsb_PPC(dest_current_value, dest_current_value); break;
      case 2: lha_PPC(dest_current_value, 0, addr_base); break;
      case 4: lwz_PPC(dest_current_value, 0, addr_base); break;
      default: ShouldNotReachHere();
    }
    cmpw_PPC(flag, dest_current_value, compare_value);
    bne_PPC(flag, failed);
  }

  // release/fence semantics
  if (semantics & MemBarRel) {
    release();
  }

  cmpxchg_loop_body(flag, dest_current_value, compare_value, exchange_value, addr_base, tmp1, tmp2,
                    retry, failed, cmpxchgx_hint, size);
  if (!weak || use_result_reg) {
    if (UseStaticBranchPredictionInCompareAndSwapRISCV64) {
      bne_predict_not_taken_PPC(CCR0, weak ? failed : retry); // StXcx_ sets CCR0.
    } else {
      bne_PPC(                  CCR0, weak ? failed : retry); // StXcx_ sets CCR0.
    }
  }
  // fall through    => (flag == eq), (dest_current_value == compare_value), (swapped)

  // Result in register (must do this at the end because int_flag_success can be the
  // same register as one above).
  if (use_result_reg) {
    li_PPC(int_flag_success, 1);
  }

  if (semantics & MemBarFenceAfter) {
    fence();
  } else if (semantics & MemBarAcq) {
    isync_PPC();
  }

  if (use_result_reg && !preset_result_reg) {
    b_PPC(done);
  }

  bind(failed);
  if (use_result_reg && !preset_result_reg) {
    li_PPC(int_flag_success, 0);
  }

  bind(done);
  // (flag == ne) => (dest_current_value != compare_value), (!swapped)
  // (flag == eq) => (dest_current_value == compare_value), ( swapped)
}

// Preforms atomic compare exchange:
//   if (compare_value == *addr_base)
//     *addr_base = exchange_value
//     int_flag_success = 1;
//   else
//     int_flag_success = 0;
//
// ConditionRegister flag       = cmp_PPC(compare_value, *addr_base)
// Register dest_current_value  = *addr_base
// Register compare_value       Used to compare with value in memory
// Register exchange_value      Written to memory if compare_value == *addr_base
// Register addr_base           The memory location to compareXChange
// Register int_flag_success    Set to 1 if exchange_value was written to *addr_base
//
// To avoid the costly compare exchange the value is tested beforehand.
// Several special cases exist to avoid that unnecessary information is generated.
//
void MacroAssembler::cmpxchgd(ConditionRegister flag,
                              Register dest_current_value, RegisterOrConstant compare_value, Register exchange_value,
                              Register addr_base, int semantics, bool cmpxchgx_hint,
                              Register int_flag_success, Label* failed_ext, bool contention_hint, bool weak) {
  Label retry;
  Label failed_int;
  Label& failed = (failed_ext != NULL) ? *failed_ext : failed_int;
  Label done;

  // Save one branch if result is returned via register and result register is different from the other ones.
  bool use_result_reg    = (int_flag_success!=noreg);
  bool preset_result_reg = (int_flag_success!=dest_current_value && int_flag_success!=compare_value.register_or_noreg() &&
                            int_flag_success!=exchange_value && int_flag_success!=addr_base);
  assert(!weak || flag == CCR0, "weak only supported with CCR0");
  assert(int_flag_success == noreg || failed_ext == NULL, "cannot have both");

  if (use_result_reg && preset_result_reg) {
    li_PPC(int_flag_success, 0); // preset (assume cas failed)
  }

  // Add simple guard in order to reduce risk of starving under high contention (recommended by IBM).
  if (contention_hint) { // Don't try to reserve if cmp fails.
    ld_PPC(dest_current_value, 0, addr_base);
    cmpd_PPC(flag, compare_value, dest_current_value);
    bne_PPC(flag, failed);
  }

  // release/fence semantics
  if (semantics & MemBarRel) {
    release();
  }

  // atomic emulation loop
  bind(retry);

  ldarx_PPC(dest_current_value, addr_base, cmpxchgx_hint);
  cmpd_PPC(flag, compare_value, dest_current_value);
  if (UseStaticBranchPredictionInCompareAndSwapRISCV64) {
    bne_predict_not_taken_PPC(flag, failed);
  } else {
    bne_PPC(                  flag, failed);
  }

  stdcx__PPC(exchange_value, addr_base);
  if (!weak || use_result_reg || failed_ext) {
    if (UseStaticBranchPredictionInCompareAndSwapRISCV64) {
      bne_predict_not_taken_PPC(CCR0, weak ? failed : retry); // stXcx_ sets CCR0
    } else {
      bne_PPC(                  CCR0, weak ? failed : retry); // stXcx_ sets CCR0
    }
  }

  // result in register (must do this at the end because int_flag_success can be the same register as one above)
  if (use_result_reg) {
    li_PPC(int_flag_success, 1);
  }

  if (semantics & MemBarFenceAfter) {
    fence();
  } else if (semantics & MemBarAcq) {
    isync_PPC();
  }

  if (use_result_reg && !preset_result_reg) {
    b_PPC(done);
  }

  bind(failed_int);
  if (use_result_reg && !preset_result_reg) {
    li_PPC(int_flag_success, 0);
  }

  bind(done);
  // (flag == ne) => (dest_current_value != compare_value), (!swapped)
  // (flag == eq) => (dest_current_value == compare_value), ( swapped)
}

void MacroAssembler::cmpxchgd_simple(Register dest_current_value, Register compare_value, Register exchange_value,
                                     Register addr_base, Register tmp, Label* failed_ext) {
  Label retry;
  Label done;

  // atomic emulation loop
  bind(retry);
  lrd(dest_current_value, addr_base);
  bne(dest_current_value, compare_value, *failed_ext);
  scd(tmp, exchange_value, addr_base);
  bnez(tmp, retry);

  bind(done);
  // (flag == ne) => (dest_current_value != compare_value), (!swapped)
  // (flag == eq) => (dest_current_value == compare_value), ( swapped)
}

void MacroAssembler::cmpxchg_for_lock_acquire(Register dest_current_value, Register compare_value, Register exchange_value,
                                              Register addr_base, Register tmp, Label* failed_ext) {
  Assembler::fence(Assembler::W_OP, Assembler::W_OP);
  cmpxchgd_simple(dest_current_value, compare_value, exchange_value, addr_base, tmp, failed_ext);
  fence();
}

void MacroAssembler::cmpxchg_for_lock_release(Register dest_current_value, Register compare_value, Register exchange_value,
                                              Register addr_base, Register tmp, Label* failed_ext) {
  fence();
  cmpxchgd_simple(dest_current_value, compare_value, exchange_value, addr_base, tmp, failed_ext);
  fence(); //Assembler::fence(Assembler::W_OP, Assembler::R_OP);
}

// Look up the method for a megamorphic invokeinterface call.
// The target method is determined by <intf_klass, itable_index>.
// The receiver klass is in recv_klass.
// On success, the result will be in method_result, and execution falls through.
// On failure, execution transfers to the given label.
void MacroAssembler::lookup_interface_method(Register recv_klass,
                                             Register intf_klass,
                                             RegisterOrConstant itable_index,
                                             Register method_result,
                                             Register scan_temp,
                                             Register temp2,
                                             Label& L_no_such_interface,
                                             bool return_method) {
  assert_different_registers(recv_klass, intf_klass, method_result, scan_temp);

  // Compute start of first itableOffsetEntry (which is at the end of the vtable).
  int vtable_base = in_bytes(Klass::vtable_start_offset());
  int itentry_off = itableMethodEntry::method_offset_in_bytes();
  int logMEsize   = exact_log2(itableMethodEntry::size() * wordSize);
  int scan_step   = itableOffsetEntry::size() * wordSize;
  int log_vte_size= exact_log2(vtableEntry::size_in_bytes());

   printf("lookup-1: %d %p\n", return_method, pc());

  lwu(scan_temp, recv_klass, in_bytes(Klass::vtable_length_offset()));
  // %%% We should store the aligned, prescaled offset in the klassoop.
  // Then the next several instructions would fold away.

  slli(scan_temp, scan_temp, log_vte_size);
  addi(scan_temp, scan_temp, vtable_base);
  add(scan_temp, recv_klass, scan_temp);

  // Adjust recv_klass by scaled itable_index, so we can free itable_index.
  if (return_method) {
    if (itable_index.is_register()) {
      Register itable_offset = itable_index.as_register();
      slli(method_result, itable_offset, logMEsize);
      if (itentry_off) { addi(method_result, method_result, itentry_off); }
      add(method_result, method_result, recv_klass);
      printf("lookup-3.1: %p\n", pc());
    } else {
	    // FixME
      long itable_offset = (long)itable_index.as_constant();
      // static address, no relocation
      add_const_optimized(method_result, recv_klass, (itable_offset << logMEsize) + itentry_off, temp2);
    }
  }

  // for (scan = klass->itable(); scan->interface() != NULL; scan += scan_step) {
  //   if (scan->interface() == intf) {
  //     result = (klass + scan->offset() + itable_index);
  //   }
  // }
  Label search, found_method;

  printf("lookup-5: %p\n", pc());
  addi(intf_klass, intf_klass, 0); // TODO RISCV DELETE THIS LINE

  for (int peel = 1; peel >= 0; peel--) {
    // %%%% Could load both offset and interface in one ldx, if they were
    // in the opposite order. This would save a load.
    ld(temp2, scan_temp, itableOffsetEntry::interface_offset_in_bytes());

    // Check that this entry is non-null. A null entry means that
    // the receiver class doesn't implement the interface, and wasn't the
    // same as when the caller was compiled.
    sub(temp2, temp2, intf_klass);
    //_PPC(CCR0, temp2, intf_klass);

    if (peel) {
      beqz(temp2, found_method);
    } else {
      unimplemented("itable loop not implemented");
      bne_PPC(CCR0, search);
      // (invert the test to fall through to found_method...)
    }

    if (!peel) break;

      unimplemented("itable loop not implemented -2");

    bind(search);

    cmpdi_PPC(CCR0, temp2, 0);
    beq_PPC(CCR0, L_no_such_interface);
    addi_PPC(scan_temp, scan_temp, scan_step);
  }

  bind(found_method);

  printf("lookup-9: %p\n", pc());


  // Got a hit.
  if (return_method) {
    int ito_offset = itableOffsetEntry::offset_offset_in_bytes();
    lw(scan_temp, scan_temp, ito_offset);
    ld(method_result, scan_temp, method_result);
  }
}

// virtual method calling
void MacroAssembler::lookup_virtual_method(Register recv_klass,
                                           RegisterOrConstant vtable_index,
                                           Register method_result) {

  assert_different_registers(recv_klass, method_result, vtable_index.register_or_noreg());

  const int base = in_bytes(Klass::vtable_start_offset());
  assert(vtableEntry::size() * wordSize == wordSize, "adjust the scaling in the code below");

  if (vtable_index.is_register()) {
    sldi_PPC(vtable_index.as_register(), vtable_index.as_register(), LogBytesPerWord);
    add_PPC(recv_klass, vtable_index.as_register(), recv_klass);
  } else {
    addi_PPC(recv_klass, recv_klass, vtable_index.as_constant() << LogBytesPerWord);
  }
  ld_PPC(R27_method, base + vtableEntry::method_offset_in_bytes(), recv_klass);
}

/////////////////////////////////////////// subtype checking ////////////////////////////////////////////
void MacroAssembler::check_klass_subtype_fast_path(Register sub_klass,
                                                   Register super_klass,
                                                   Register temp1_reg,
                                                   Register temp2_reg,
                                                   Label* L_success,
                                                   Label* L_failure,
                                                   Label* L_slow_path,
                                                   RegisterOrConstant super_check_offset) {

  const Register check_cache_offset = temp1_reg;
  const Register cached_super       = temp2_reg;

  assert_different_registers(sub_klass, super_klass, check_cache_offset, cached_super);

  int sco_offset = in_bytes(Klass::super_check_offset_offset());
  int sc_offset  = in_bytes(Klass::secondary_super_cache_offset());

  bool must_load_sco = (super_check_offset.constant_or_zero() == -1);
  bool need_slow_path = (must_load_sco || super_check_offset.constant_or_zero() == sco_offset);

  Label L_fallthrough;
  int label_nulls = 0;
  if (L_success == NULL)   { L_success   = &L_fallthrough; label_nulls++; }
  if (L_failure == NULL)   { L_failure   = &L_fallthrough; label_nulls++; }
  if (L_slow_path == NULL) { L_slow_path = &L_fallthrough; label_nulls++; }
  assert(label_nulls <= 1 ||
         (L_slow_path == &L_fallthrough && label_nulls <= 2 && !need_slow_path),
         "at most one NULL in the batch, usually");

  // If the pointers are equal, we are done (e.g., String[] elements).
  // This self-check enables sharing of secondary supertype arrays among
  // non-primary types such as array-of-interface. Otherwise, each such
  // type would need its own customized SSA.
  // We move this check to the front of the fast path because many
  // type checks are in fact trivially successful in this manner,
  // so we get a nicely predicted branch right at the start of the check.
  cmpd_PPC(CCR0, sub_klass, super_klass);
  beq_PPC(CCR0, *L_success);

  // Check the supertype display:
  if (must_load_sco) {
    // The super check offset is always positive...
    lwz_PPC(check_cache_offset, sco_offset, super_klass);
    super_check_offset = RegisterOrConstant(check_cache_offset);
    // super_check_offset is register.
    assert_different_registers(sub_klass, super_klass, cached_super, super_check_offset.as_register());
  }
  // The loaded value is the offset from KlassOopDesc.

  ld_PPC(cached_super, super_check_offset, sub_klass);
  cmpd_PPC(CCR0, cached_super, super_klass);

  // This check has worked decisively for primary supers.
  // Secondary supers are sought in the super_cache ('super_cache_addr').
  // (Secondary supers are interfaces and very deeply nested subtypes.)
  // This works in the same check above because of a tricky aliasing
  // between the super_cache and the primary super display elements.
  // (The 'super_check_addr' can address either, as the case requires.)
  // Note that the cache is updated below if it does not help us find
  // what we need immediately.
  // So if it was a primary super, we can just fail immediately.
  // Otherwise, it's the slow path for us (no success at this point).

#define FINAL_JUMP(label) if (&(label) != &L_fallthrough) { b_PPC(label); }

  if (super_check_offset.is_register()) {
    beq_PPC(CCR0, *L_success);
    cmpwi_PPC(CCR0, super_check_offset.as_register(), sc_offset);
    if (L_failure == &L_fallthrough) {
      beq_PPC(CCR0, *L_slow_path);
    } else {
      bne_PPC(CCR0, *L_failure);
      FINAL_JUMP(*L_slow_path);
    }
  } else {
    if (super_check_offset.as_constant() == sc_offset) {
      // Need a slow path; fast failure is impossible.
      if (L_slow_path == &L_fallthrough) {
        beq_PPC(CCR0, *L_success);
      } else {
        bne_PPC(CCR0, *L_slow_path);
        FINAL_JUMP(*L_success);
      }
    } else {
      // No slow path; it's a fast decision.
      if (L_failure == &L_fallthrough) {
        beq_PPC(CCR0, *L_success);
      } else {
        bne_PPC(CCR0, *L_failure);
        FINAL_JUMP(*L_success);
      }
    }
  }

  bind(L_fallthrough);
#undef FINAL_JUMP
}

void MacroAssembler::check_klass_subtype_slow_path(Register sub_klass,
                                                   Register super_klass,
                                                   Register temp1_reg,
                                                   Register temp2_reg,
                                                   Label* L_success,
                                                   Register result_reg) {
  const Register array_ptr = temp1_reg; // current value from cache array
  const Register temp      = temp2_reg;

  assert_different_registers(sub_klass, super_klass, array_ptr, temp);

  int source_offset = in_bytes(Klass::secondary_supers_offset());
  int target_offset = in_bytes(Klass::secondary_super_cache_offset());

  int length_offset = Array<Klass*>::length_offset_in_bytes();
  int base_offset   = Array<Klass*>::base_offset_in_bytes();

  Label hit, loop, failure, fallthru;

  ld_PPC(array_ptr, source_offset, sub_klass);

  // TODO: RISCV port: assert(4 == arrayOopDesc::length_length_in_bytes(), "precondition violated.");
  lwz_PPC(temp, length_offset, array_ptr);
  cmpwi_PPC(CCR0, temp, 0);
  beq_PPC(CCR0, result_reg!=noreg ? failure : fallthru); // length 0

  mtctr_PPC(temp); // load ctr

  bind(loop);
  // Oops in table are NO MORE compressed.
  ld_PPC(temp, base_offset, array_ptr);
  cmpd_PPC(CCR0, temp, super_klass);
  beq_PPC(CCR0, hit);
  addi_PPC(array_ptr, array_ptr, BytesPerWord);
  bdnz_PPC(loop);

  bind(failure);
  if (result_reg!=noreg) li_PPC(result_reg, 1); // load non-zero result (indicates a miss)
  b_PPC(fallthru);

  bind(hit);
  std_PPC(super_klass, target_offset, sub_klass); // save result to cache
  if (result_reg != noreg) { li_PPC(result_reg, 0); } // load zero result (indicates a hit)
  if (L_success != NULL) { b_PPC(*L_success); }
  else if (result_reg == noreg) { blr_PPC(); } // return with CR0.eq if neither label nor result reg provided

  bind(fallthru);
}

// Try fast path, then go to slow one if not successful
void MacroAssembler::check_klass_subtype(Register sub_klass,
                         Register super_klass,
                         Register temp1_reg,
                         Register temp2_reg,
                         Label& L_success) {
  Label L_failure;
  check_klass_subtype_fast_path(sub_klass, super_klass, temp1_reg, temp2_reg, &L_success, &L_failure);
  check_klass_subtype_slow_path(sub_klass, super_klass, temp1_reg, temp2_reg, &L_success);
  bind(L_failure); // Fallthru if not successful.
}

void MacroAssembler::clinit_barrier(Register klass, Register thread, Label* L_fast_path, Label* L_slow_path) {
  assert(L_fast_path != NULL || L_slow_path != NULL, "at least one is required");

  Label L_fallthrough;
  if (L_fast_path == NULL) {
    L_fast_path = &L_fallthrough;
  } else if (L_slow_path == NULL) {
    L_slow_path = &L_fallthrough;
  }

  // Fast path check: class is fully initialized
  lbu(R29_TMP4, klass, in_bytes(InstanceKlass::init_state_offset()));
  li(R30_TMP5, InstanceKlass::fully_initialized);
  beq(R30_TMP5, R29_TMP4, *L_fast_path);

  // Fast path check: current thread is initializer thread
  ld(R29_TMP4, klass, in_bytes(InstanceKlass::init_thread_offset()));
  if (L_slow_path == &L_fallthrough) {
    beq(R29_TMP4, thread, *L_fast_path);
  } else if (L_fast_path == &L_fallthrough) {
    bne(R29_TMP4, thread, *L_slow_path);
  } else {
    Unimplemented();
  }

  bind(L_fallthrough);
}

void MacroAssembler::check_method_handle_type(Register mtype_reg, Register mh_reg,
                                              Register temp_reg,
                                              Label& wrong_method_type) {
  assert_different_registers(mtype_reg, mh_reg, temp_reg);
  // Compare method type against that of the receiver.
  load_heap_oop(temp_reg, delayed_value(java_lang_invoke_MethodHandle::type_offset_in_bytes, temp_reg), mh_reg,
                noreg, noreg, false, IS_NOT_NULL);
  cmpd_PPC(CCR0, temp_reg, mtype_reg);
  bne_PPC(CCR0, wrong_method_type);
}

RegisterOrConstant MacroAssembler::argument_offset(RegisterOrConstant arg_slot,
                                                   Register temp_reg,
                                                   int extra_slot_offset) {
  // cf. TemplateTable::prepare_invoke(), if (load_receiver).
  int stackElementSize = Interpreter::stackElementSize;
  int offset = extra_slot_offset * stackElementSize;
  if (arg_slot.is_constant()) {
    offset += arg_slot.as_constant() * stackElementSize;
    return offset;
  } else {
    assert(temp_reg != noreg, "must specify");
    sldi_PPC(temp_reg, arg_slot.as_register(), exact_log2(stackElementSize));
    if (offset != 0)
      addi_PPC(temp_reg, temp_reg, offset);
    return temp_reg;
  }
}

// Supports temp2_reg = R0.
void MacroAssembler::biased_locking_enter(ConditionRegister cr_reg, Register obj_reg,
                                          Register mark_reg, Register temp_reg,
                                          Register temp2_reg, Label& done, Label* slow_case) {
  assert(UseBiasedLocking, "why call this otherwise?");

#ifdef ASSERT
  assert_different_registers(obj_reg, mark_reg, temp_reg, temp2_reg);
#endif

  Label cas_label;

  // Branch to done if fast path fails and no slow_case provided.
  Label *slow_case_int = (slow_case != NULL) ? slow_case : &done;

  // Biased locking
  // See whether the lock is currently biased toward our thread and
  // whether the epoch is still valid
  // Note that the runtime guarantees sufficient alignment of JavaThread
  // pointers to allow age to be placed into low bits
  assert(markOopDesc::age_shift == markOopDesc::lock_bits + markOopDesc::biased_lock_bits,
         "biased locking makes assumptions about bit layout");

  if (PrintBiasedLockingStatistics) {
    load_const(temp2_reg, (address) BiasedLocking::total_entry_count_addr(), temp_reg);
    lwzx_PPC(temp_reg, temp2_reg);
    addi_PPC(temp_reg, temp_reg, 1);
    stwx_PPC(temp_reg, temp2_reg);
  }

  andi_PPC(temp_reg, mark_reg, markOopDesc::biased_lock_mask_in_place);
  cmpwi_PPC(cr_reg, temp_reg, markOopDesc::biased_lock_pattern);
  bne_PPC(cr_reg, cas_label);

  load_klass(temp_reg, obj_reg);

  load_const_optimized(temp2_reg, ~((int) markOopDesc::age_mask_in_place));
  ld_PPC(temp_reg, in_bytes(Klass::prototype_header_offset()), temp_reg);
  orr_PPC(temp_reg, R24_thread, temp_reg);
  xorr_PPC(temp_reg, mark_reg, temp_reg);
  andr_PPC(temp_reg, temp_reg, temp2_reg);
  cmpdi_PPC(cr_reg, temp_reg, 0);
  if (PrintBiasedLockingStatistics) {
    Label l;
    bne_PPC(cr_reg, l);
    load_const(temp2_reg, (address) BiasedLocking::biased_lock_entry_count_addr());
    lwzx_PPC(mark_reg, temp2_reg);
    addi_PPC(mark_reg, mark_reg, 1);
    stwx_PPC(mark_reg, temp2_reg);
    // restore mark_reg
    ld_PPC(mark_reg, oopDesc::mark_offset_in_bytes(), obj_reg);
    bind(l);
  }
  beq_PPC(cr_reg, done);

  Label try_revoke_bias;
  Label try_rebias;

  // At this point we know that the header has the bias pattern and
  // that we are not the bias owner in the current epoch. We need to
  // figure out more details about the state of the header in order to
  // know what operations can be legally performed on the object's
  // header.

  // If the low three bits in the xor result aren't clear, that means
  // the prototype header is no longer biased and we have to revoke
  // the bias on this object.
  andi_PPC(temp2_reg, temp_reg, markOopDesc::biased_lock_mask_in_place);
  cmpwi_PPC(cr_reg, temp2_reg, 0);
  bne_PPC(cr_reg, try_revoke_bias);

  // Biasing is still enabled for this data type. See whether the
  // epoch of the current bias is still valid, meaning that the epoch
  // bits of the mark word are equal to the epoch bits of the
  // prototype header. (Note that the prototype header's epoch bits
  // only change at a safepoint.) If not, attempt to rebias the object
  // toward the current thread. Note that we must be absolutely sure
  // that the current epoch is invalid in order to do this because
  // otherwise the manipulations it performs on the mark word are
  // illegal.

  int shift_amount = 64 - markOopDesc::epoch_shift;
  // rotate epoch bits to right (little) end and set other bits to 0
  // [ big part | epoch | little part ] -> [ 0..0 | epoch ]
  rldicl__PPC(temp2_reg, temp_reg, shift_amount, 64 - markOopDesc::epoch_bits);
  // branch if epoch bits are != 0, i.e. they differ, because the epoch has been incremented
  bne_PPC(CCR0, try_rebias);

  // The epoch of the current bias is still valid but we know nothing
  // about the owner; it might be set or it might be clear. Try to
  // acquire the bias of the object using an atomic operation. If this
  // fails we will go in to the runtime to revoke the object's bias.
  // Note that we first construct the presumed unbiased header so we
  // don't accidentally blow away another thread's valid bias.
  andi_PPC(mark_reg, mark_reg, (markOopDesc::biased_lock_mask_in_place |
                                markOopDesc::age_mask_in_place |
                                markOopDesc::epoch_mask_in_place));
  orr_PPC(temp_reg, R24_thread, mark_reg);

  assert(oopDesc::mark_offset_in_bytes() == 0, "offset of _mark is not 0");

  // CmpxchgX sets cr_reg to cmpX(temp2_reg, mark_reg).
  cmpxchgd(/*flag=*/cr_reg, /*current_value=*/temp2_reg,
           /*compare_value=*/mark_reg, /*exchange_value=*/temp_reg,
           /*where=*/obj_reg,
           MacroAssembler::MemBarAcq,
           MacroAssembler::cmpxchgx_hint_acquire_lock(),
           noreg, slow_case_int); // bail out if failed

  // If the biasing toward our thread failed, this means that
  // another thread succeeded in biasing it toward itself and we
  // need to revoke that bias. The revocation will occur in the
  // interpreter runtime in the slow case.
  if (PrintBiasedLockingStatistics) {
    load_const(temp2_reg, (address) BiasedLocking::anonymously_biased_lock_entry_count_addr(), temp_reg);
    lwzx_PPC(temp_reg, temp2_reg);
    addi_PPC(temp_reg, temp_reg, 1);
    stwx_PPC(temp_reg, temp2_reg);
  }
  b_PPC(done);

  bind(try_rebias);
  // At this point we know the epoch has expired, meaning that the
  // current "bias owner", if any, is actually invalid. Under these
  // circumstances _only_, we are allowed to use the current header's
  // value as the comparison value when doing the cas to acquire the
  // bias in the current epoch. In other words, we allow transfer of
  // the bias from one thread to another directly in this situation.
  load_klass(temp_reg, obj_reg);
  andi_PPC(temp2_reg, mark_reg, markOopDesc::age_mask_in_place);
  orr_PPC(temp2_reg, R24_thread, temp2_reg);
  ld_PPC(temp_reg, in_bytes(Klass::prototype_header_offset()), temp_reg);
  orr_PPC(temp_reg, temp2_reg, temp_reg);

  assert(oopDesc::mark_offset_in_bytes() == 0, "offset of _mark is not 0");

  cmpxchgd(/*flag=*/cr_reg, /*current_value=*/temp2_reg,
                 /*compare_value=*/mark_reg, /*exchange_value=*/temp_reg,
                 /*where=*/obj_reg,
                 MacroAssembler::MemBarAcq,
                 MacroAssembler::cmpxchgx_hint_acquire_lock(),
                 noreg, slow_case_int); // bail out if failed

  // If the biasing toward our thread failed, this means that
  // another thread succeeded in biasing it toward itself and we
  // need to revoke that bias. The revocation will occur in the
  // interpreter runtime in the slow case.
  if (PrintBiasedLockingStatistics) {
    load_const(temp2_reg, (address) BiasedLocking::rebiased_lock_entry_count_addr(), temp_reg);
    lwzx_PPC(temp_reg, temp2_reg);
    addi_PPC(temp_reg, temp_reg, 1);
    stwx_PPC(temp_reg, temp2_reg);
  }
  b_PPC(done);

  bind(try_revoke_bias);
  // The prototype mark in the klass doesn't have the bias bit set any
  // more, indicating that objects of this data type are not supposed
  // to be biased any more. We are going to try to reset the mark of
  // this object to the prototype value and fall through to the
  // CAS-based locking scheme. Note that if our CAS fails, it means
  // that another thread raced us for the privilege of revoking the
  // bias of this particular object, so it's okay to continue in the
  // normal locking code.
  load_klass(temp_reg, obj_reg);
  ld_PPC(temp_reg, in_bytes(Klass::prototype_header_offset()), temp_reg);
  andi_PPC(temp2_reg, mark_reg, markOopDesc::age_mask_in_place);
  orr_PPC(temp_reg, temp_reg, temp2_reg);

  assert(oopDesc::mark_offset_in_bytes() == 0, "offset of _mark is not 0");

  // CmpxchgX sets cr_reg to cmpX(temp2_reg, mark_reg).
  cmpxchgd(/*flag=*/cr_reg, /*current_value=*/temp2_reg,
                 /*compare_value=*/mark_reg, /*exchange_value=*/temp_reg,
                 /*where=*/obj_reg,
                 MacroAssembler::MemBarAcq,
                 MacroAssembler::cmpxchgx_hint_acquire_lock());

  // reload markOop in mark_reg before continuing with lightweight locking
  ld_PPC(mark_reg, oopDesc::mark_offset_in_bytes(), obj_reg);

  // Fall through to the normal CAS-based lock, because no matter what
  // the result of the above CAS, some thread must have succeeded in
  // removing the bias bit from the object's header.
  if (PrintBiasedLockingStatistics) {
    Label l;
    bne_PPC(cr_reg, l);
    load_const(temp2_reg, (address) BiasedLocking::revoked_lock_entry_count_addr(), temp_reg);
    lwzx_PPC(temp_reg, temp2_reg);
    addi_PPC(temp_reg, temp_reg, 1);
    stwx_PPC(temp_reg, temp2_reg);
    bind(l);
  }

  bind(cas_label);
}

void MacroAssembler::biased_locking_exit (ConditionRegister cr_reg, Register mark_addr, Register temp_reg, Label& done) {
  // Check for biased locking unlock case, which is a no-op
  // Note: we do not have to check the thread ID for two reasons.
  // First, the interpreter checks for IllegalMonitorStateException at
  // a higher level. Second, if the bias was revoked while we held the
  // lock, the object could not be rebiased toward another thread, so
  // the bias bit would be clear.

  ld_PPC(temp_reg, 0, mark_addr);
  andi_PPC(temp_reg, temp_reg, markOopDesc::biased_lock_mask_in_place);

  cmpwi_PPC(cr_reg, temp_reg, markOopDesc::biased_lock_pattern);
  beq_PPC(cr_reg, done);
}

// allocation (for C1)
void MacroAssembler::eden_allocate(
  Register obj,                      // result: pointer to object after successful allocation
  Register var_size_in_bytes,        // object size in bytes if unknown at compile time; invalid otherwise
  int      con_size_in_bytes,        // object size in bytes if   known at compile time
  Register t1,                       // temp register
  Register t2,                       // temp register
  Label&   slow_case                 // continuation point if fast allocation fails
) {
  b_PPC(slow_case);
}

void MacroAssembler::tlab_allocate(
  Register obj,                      // result: pointer to object after successful allocation
  Register var_size_in_bytes,        // object size in bytes if unknown at compile time; invalid otherwise
  int      con_size_in_bytes,        // object size in bytes if   known at compile time
  Register t1,                       // temp register
  Label&   slow_case                 // continuation point if fast allocation fails
) {
  // make sure arguments make sense
  assert_different_registers(obj, var_size_in_bytes, t1);
  assert(0 <= con_size_in_bytes && is_simm16(con_size_in_bytes), "illegal object size");
  assert((con_size_in_bytes & MinObjAlignmentInBytesMask) == 0, "object size is not multiple of alignment");

  const Register new_top = t1;
  //verify_tlab(); not implemented

  ld_PPC(obj, in_bytes(JavaThread::tlab_top_offset()), R24_thread);
  ld_PPC(R0, in_bytes(JavaThread::tlab_end_offset()), R24_thread);
  if (var_size_in_bytes == noreg) {
    addi_PPC(new_top, obj, con_size_in_bytes);
  } else {
    add_PPC(new_top, obj, var_size_in_bytes);
  }
  cmpld_PPC(CCR0, new_top, R0);
  bc_far_optimized(Assembler::bcondCRbiIs1, bi0(CCR0, Assembler::greater), slow_case);

#ifdef ASSERT
  // make sure new free pointer is properly aligned
  {
    Label L;
    andi__PPC(R0, new_top, MinObjAlignmentInBytesMask);
    beq_PPC(CCR0, L);
    stop("updated TLAB free is not properly aligned", 0x934);
    bind(L);
  }
#endif // ASSERT

  // update the tlab top pointer
  std_PPC(new_top, in_bytes(JavaThread::tlab_top_offset()), R24_thread);
  //verify_tlab(); not implemented
}
void MacroAssembler::incr_allocated_bytes(RegisterOrConstant size_in_bytes, Register t1, Register t2) {
  unimplemented("incr_allocated_bytes");
}

address MacroAssembler::emit_trampoline_stub(int destination_toc_offset,
                                             int insts_call_instruction_offset, Register Rtoc) {
  // Start the stub.
  address stub = start_a_stub(64);
  if (stub == NULL) { return NULL; } // CodeCache full: bail out

  // Create a trampoline stub relocation which relates this trampoline stub
  // with the call instruction at insts_call_instruction_offset in the
  // instructions code-section.
  relocate(trampoline_stub_Relocation::spec(code()->insts()->start() + insts_call_instruction_offset));
  const int stub_start_offset = offset();

  // For java_to_interp stubs we use R5_scratch1 as scratch register
  // and in call trampoline stubs we use R6_scratch2. This way we
  // can distinguish them (see is_NativeCallTrampolineStub_at()).
  Register reg_scratch = R6_scratch2;

  // Now, create the trampoline stub's code:
  // - load the TOC
  // - load the call target from the constant pool
  // - call
  if (Rtoc == noreg) {
    calculate_address_from_global_toc(reg_scratch, method_toc());
    Rtoc = reg_scratch;
  }

  ld_largeoffset_unchecked(reg_scratch, destination_toc_offset, Rtoc, false);
  mtctr_PPC(reg_scratch);
  bctr_PPC();

  const address stub_start_addr = addr_at(stub_start_offset);

  // Assert that the encoded destination_toc_offset can be identified and that it is correct.
  assert(destination_toc_offset == NativeCallTrampolineStub_at(stub_start_addr)->destination_toc_offset(),
         "encoded offset into the constant pool must match");
  // Trampoline_stub_size should be good.
  assert((uint)(offset() - stub_start_offset) <= trampoline_stub_size, "should be good size");
  assert(is_NativeCallTrampolineStub_at(stub_start_addr), "doesn't look like a trampoline");

  // End the stub.
  end_a_stub();
  return stub;
}

// TM on RISCV64.
void MacroAssembler::atomic_inc_ptr(Register addr, Register result, int simm16) {
  Label retry;
  bind(retry);
  ldarx_PPC(result, addr, /*hint*/ false);
  addi_PPC(result, result, simm16);
  stdcx__PPC(result, addr);
  if (UseStaticBranchPredictionInCompareAndSwapRISCV64) {
    bne_predict_not_taken_PPC(CCR0, retry); // stXcx_ sets CCR0
  } else {
    bne_PPC(                  CCR0, retry); // stXcx_ sets CCR0
  }
}

void MacroAssembler::atomic_ori_int(Register addr, Register result, int uimm16) {
  Label retry;
  bind(retry);
  lwarx_PPC(result, addr, /*hint*/ false);
  ori_PPC(result, result, uimm16);
  stwcx__PPC(result, addr);
  if (UseStaticBranchPredictionInCompareAndSwapRISCV64) {
    bne_predict_not_taken_PPC(CCR0, retry); // stXcx_ sets CCR0
  } else {
    bne_PPC(                  CCR0, retry); // stXcx_ sets CCR0
  }
}

#if INCLUDE_RTM_OPT

// Update rtm_counters based on abort status
// input: abort_status
//        rtm_counters_Reg (RTMLockingCounters*)
void MacroAssembler::rtm_counters_update(Register abort_status, Register rtm_counters_Reg) {
  // Mapping to keep PreciseRTMLockingStatistics similar to x86.
  // x86 riscv (! means inverted, ? means not the same)
  //  0   31  Set if abort caused by XABORT instruction.
  //  1  ! 7  If set, the transaction may succeed on a retry. This bit is always clear if bit 0 is set.
  //  2   13  Set if another logical processor conflicted with a memory address that was part of the transaction that aborted.
  //  3   10  Set if an internal buffer overflowed.
  //  4  ?12  Set if a debug breakpoint was hit.
  //  5  ?32  Set if an abort occurred during execution of a nested transaction.
  const int failure_bit[] = {tm_tabort, // Signal handler will set this too.
                             tm_failure_persistent,
                             tm_non_trans_cf,
                             tm_trans_cf,
                             tm_footprint_of,
                             tm_failure_code,
                             tm_transaction_level};

  const int num_failure_bits = sizeof(failure_bit) / sizeof(int);
  const int num_counters = RTMLockingCounters::ABORT_STATUS_LIMIT;

  const int bit2counter_map[][num_counters] =
  // 0 = no map; 1 = mapped, no inverted logic; -1 = mapped, inverted logic
  // Inverted logic means that if a bit is set don't count it, or vice-versa.
  // Care must be taken when mapping bits to counters as bits for a given
  // counter must be mutually exclusive. Otherwise, the counter will be
  // incremented more than once.
  // counters:
  // 0        1        2         3         4         5
  // abort  , persist, conflict, overflow, debug   , nested         bits:
  {{ 1      , 0      , 0       , 0       , 0       , 0      },   // abort
   { 0      , -1     , 0       , 0       , 0       , 0      },   // failure_persistent
   { 0      , 0      , 1       , 0       , 0       , 0      },   // non_trans_cf
   { 0      , 0      , 1       , 0       , 0       , 0      },   // trans_cf
   { 0      , 0      , 0       , 1       , 0       , 0      },   // footprint_of
   { 0      , 0      , 0       , 0       , -1      , 0      },   // failure_code = 0xD4
   { 0      , 0      , 0       , 0       , 0       , 1      }};  // transaction_level > 1
  // ...

  // Move abort_status value to R0 and use abort_status register as a
  // temporary register because R0 as third operand in ld/std is treated
  // as base address zero (value). Likewise, R0 as second operand in addi
  // is problematic because it amounts to li.
  const Register temp_Reg = abort_status;
  const Register abort_status_R0 = R0;
  mr_PPC(abort_status_R0, abort_status);

  // Increment total abort counter.
  int counters_offs = RTMLockingCounters::abort_count_offset();
  ld_PPC(temp_Reg, counters_offs, rtm_counters_Reg);
  addi_PPC(temp_Reg, temp_Reg, 1);
  std_PPC(temp_Reg, counters_offs, rtm_counters_Reg);

  // Increment specific abort counters.
  if (PrintPreciseRTMLockingStatistics) {

    // #0 counter offset.
    int abortX_offs = RTMLockingCounters::abortX_count_offset();

    for (int nbit = 0; nbit < num_failure_bits; nbit++) {
      for (int ncounter = 0; ncounter < num_counters; ncounter++) {
        if (bit2counter_map[nbit][ncounter] != 0) {
          Label check_abort;
          int abort_counter_offs = abortX_offs + (ncounter << 3);

          if (failure_bit[nbit] == tm_transaction_level) {
            // Don't check outer transaction, TL = 1 (bit 63). Hence only
            // 11 bits in the TL field are checked to find out if failure
            // occured in a nested transaction. This check also matches
            // the case when nesting_of = 1 (nesting overflow).
            rldicr__PPC(temp_Reg, abort_status_R0, failure_bit[nbit], 10);
          } else if (failure_bit[nbit] == tm_failure_code) {
            // Check failure code for trap or illegal caught in TM.
            // Bits 0:7 are tested as bit 7 (persistent) is copied from
            // tabort or treclaim source operand.
            // On Linux: trap or illegal is TM_CAUSE_SIGNAL (0xD4).
            rldicl_PPC(temp_Reg, abort_status_R0, 8, 56);
            cmpdi_PPC(CCR0, temp_Reg, 0xD4);
          } else {
            rldicr__PPC(temp_Reg, abort_status_R0, failure_bit[nbit], 0);
          }

          if (bit2counter_map[nbit][ncounter] == 1) {
            beq_PPC(CCR0, check_abort);
          } else {
            bne_PPC(CCR0, check_abort);
          }

          // We don't increment atomically.
          ld_PPC(temp_Reg, abort_counter_offs, rtm_counters_Reg);
          addi_PPC(temp_Reg, temp_Reg, 1);
          std_PPC(temp_Reg, abort_counter_offs, rtm_counters_Reg);

          bind(check_abort);
        }
      }
    }
  }
  // Restore abort_status.
  mr_PPC(abort_status, abort_status_R0);
}

// Branch if (random & (count-1) != 0), count is 2^n
// tmp and CR0 are killed
void MacroAssembler::branch_on_random_using_tb(Register tmp, int count, Label& brLabel) {
  mftb_PPC(tmp);
  andi__PPC(tmp, tmp, count-1);
  bne_PPC(CCR0, brLabel);
}

// Perform abort ratio calculation, set no_rtm bit if high ratio.
// input:  rtm_counters_Reg (RTMLockingCounters* address) - KILLED
void MacroAssembler::rtm_abort_ratio_calculation(Register rtm_counters_Reg,
                                                 RTMLockingCounters* rtm_counters,
                                                 Metadata* method_data) {
  Label L_done, L_check_always_rtm1, L_check_always_rtm2;

  if (RTMLockingCalculationDelay > 0) {
    // Delay calculation.
    ld_PPC(rtm_counters_Reg, (RegisterOrConstant)(intptr_t)RTMLockingCounters::rtm_calculation_flag_addr());
    cmpdi_PPC(CCR0, rtm_counters_Reg, 0);
    beq_PPC(CCR0, L_done);
    load_const_optimized(rtm_counters_Reg, (address)rtm_counters, R0); // reload
  }
  // Abort ratio calculation only if abort_count > RTMAbortThreshold.
  //   Aborted transactions = abort_count * 100
  //   All transactions = total_count *  RTMTotalCountIncrRate
  //   Set no_rtm bit if (Aborted transactions >= All transactions * RTMAbortRatio)
  ld_PPC(R0, RTMLockingCounters::abort_count_offset(), rtm_counters_Reg);
  if (is_simm(RTMAbortThreshold, 16)) {   // cmpdi can handle 16bit immediate only.
    cmpdi_PPC(CCR0, R0, RTMAbortThreshold);
    blt_PPC(CCR0, L_check_always_rtm2);  // reload of rtm_counters_Reg not necessary
  } else {
    load_const_optimized(rtm_counters_Reg, RTMAbortThreshold);
    cmpd_PPC(CCR0, R0, rtm_counters_Reg);
    blt_PPC(CCR0, L_check_always_rtm1);  // reload of rtm_counters_Reg required
  }
  mulli_PPC(R0, R0, 100);

  const Register tmpReg = rtm_counters_Reg;
  ld_PPC(tmpReg, RTMLockingCounters::total_count_offset(), rtm_counters_Reg);
  mulli_PPC(tmpReg, tmpReg, RTMTotalCountIncrRate); // allowable range: int16
  mulli_PPC(tmpReg, tmpReg, RTMAbortRatio);         // allowable range: int16
  cmpd_PPC(CCR0, R0, tmpReg);
  blt_PPC(CCR0, L_check_always_rtm1); // jump to reload
  if (method_data != NULL) {
    // Set rtm_state to "no rtm" in MDO.
    // Not using a metadata relocation. Method and Class Loader are kept alive anyway.
    // (See nmethod::metadata_do and CodeBuffer::finalize_oop_references.)
    load_const_PPC(R0, (address)method_data + MethodData::rtm_state_offset_in_bytes(), tmpReg);
    atomic_ori_int(R0, tmpReg, NoRTM);
  }
  b_PPC(L_done);

  bind(L_check_always_rtm1);
  load_const_optimized(rtm_counters_Reg, (address)rtm_counters, R0); // reload
  bind(L_check_always_rtm2);
  ld_PPC(tmpReg, RTMLockingCounters::total_count_offset(), rtm_counters_Reg);
  int64_t thresholdValue = RTMLockingThreshold / RTMTotalCountIncrRate;
  if (is_simm(thresholdValue, 16)) {   // cmpdi can handle 16bit immediate only.
    cmpdi_PPC(CCR0, tmpReg, thresholdValue);
  } else {
    load_const_optimized(R0, thresholdValue);
    cmpd_PPC(CCR0, tmpReg, R0);
  }
  blt_PPC(CCR0, L_done);
  if (method_data != NULL) {
    // Set rtm_state to "always rtm" in MDO.
    // Not using a metadata relocation. See above.
    load_const_PPC(R0, (address)method_data + MethodData::rtm_state_offset_in_bytes(), tmpReg);
    atomic_ori_int(R0, tmpReg, UseRTM);
  }
  bind(L_done);
}

// Update counters and perform abort ratio calculation.
// input: abort_status_Reg
void MacroAssembler::rtm_profiling(Register abort_status_Reg, Register temp_Reg,
                                   RTMLockingCounters* rtm_counters,
                                   Metadata* method_data,
                                   bool profile_rtm) {

  assert(rtm_counters != NULL, "should not be NULL when profiling RTM");
  // Update rtm counters based on state at abort.
  // Reads abort_status_Reg, updates flags.
  assert_different_registers(abort_status_Reg, temp_Reg);
  load_const_optimized(temp_Reg, (address)rtm_counters, R0);
  rtm_counters_update(abort_status_Reg, temp_Reg);
  if (profile_rtm) {
    assert(rtm_counters != NULL, "should not be NULL when profiling RTM");
    rtm_abort_ratio_calculation(temp_Reg, rtm_counters, method_data);
  }
}

// Retry on abort if abort's status indicates non-persistent failure.
// inputs: retry_count_Reg
//       : abort_status_Reg
// output: retry_count_Reg decremented by 1
void MacroAssembler::rtm_retry_lock_on_abort(Register retry_count_Reg, Register abort_status_Reg,
                                             Label& retryLabel, Label* checkRetry) {
  Label doneRetry;

  // Don't retry if failure is persistent.
  // The persistent bit is set when a (A) Disallowed operation is performed in
  // transactional state, like for instance trying to write the TFHAR after a
  // transaction is started; or when there is (B) a Nesting Overflow (too many
  // nested transactions); or when (C) the Footprint overflows (too many
  // addressess touched in TM state so there is no more space in the footprint
  // area to track them); or in case of (D) a Self-Induced Conflict, i.e. a
  // store is performed to a given address in TM state, then once in suspended
  // state the same address is accessed. Failure (A) is very unlikely to occur
  // in the JVM. Failure (D) will never occur because Suspended state is never
  // used in the JVM. Thus mostly (B) a Nesting Overflow or (C) a Footprint
  // Overflow will set the persistent bit.
  rldicr__PPC(R0, abort_status_Reg, tm_failure_persistent, 0);
  bne_PPC(CCR0, doneRetry);

  // Don't retry if transaction was deliberately aborted, i.e. caused by a
  // tabort instruction.
  rldicr__PPC(R0, abort_status_Reg, tm_tabort, 0);
  bne_PPC(CCR0, doneRetry);

  // Retry if transaction aborted due to a conflict with another thread.
  if (checkRetry) { bind(*checkRetry); }
  addic__PPC(retry_count_Reg, retry_count_Reg, -1);
  blt_PPC(CCR0, doneRetry);
  b_PPC(retryLabel);
  bind(doneRetry);
}

// Spin and retry if lock is busy.
// inputs: owner_addr_Reg (monitor address)
//       : retry_count_Reg
// output: retry_count_Reg decremented by 1
// CTR is killed
void MacroAssembler::rtm_retry_lock_on_busy(Register retry_count_Reg, Register owner_addr_Reg, Label& retryLabel) {
  Label SpinLoop, doneRetry, doRetry;
  addic__PPC(retry_count_Reg, retry_count_Reg, -1);
  blt_PPC(CCR0, doneRetry);

  if (RTMSpinLoopCount > 1) {
    li_PPC(R0, RTMSpinLoopCount);
    mtctr_PPC(R0);
  }

  // low thread priority
  smt_prio_low_PPC();
  bind(SpinLoop);

  if (RTMSpinLoopCount > 1) {
    bdz_PPC(doRetry);
    ld_PPC(R0, 0, owner_addr_Reg);
    cmpdi_PPC(CCR0, R0, 0);
    bne_PPC(CCR0, SpinLoop);
  }

  bind(doRetry);

  // restore thread priority to default in userspace
#ifdef LINUX
  smt_prio_medium_low_PPC();
#else
  smt_prio_medium_PPC();
#endif

  b_PPC(retryLabel);

  bind(doneRetry);
}

// Use RTM for normal stack locks.
// Input: objReg (object to lock)
void MacroAssembler::rtm_stack_locking(ConditionRegister flag,
                                       Register obj, Register mark_word, Register tmp,
                                       Register retry_on_abort_count_Reg,
                                       RTMLockingCounters* stack_rtm_counters,
                                       Metadata* method_data, bool profile_rtm,
                                       Label& DONE_LABEL, Label& IsInflated) {
  assert(UseRTMForStackLocks, "why call this otherwise?");
  assert(!UseBiasedLocking, "Biased locking is not supported with RTM locking");
  Label L_rtm_retry, L_decrement_retry, L_on_abort;

  if (RTMRetryCount > 0) {
    load_const_optimized(retry_on_abort_count_Reg, RTMRetryCount); // Retry on abort
    bind(L_rtm_retry);
  }
  andi__PPC(R0, mark_word, markOopDesc::monitor_value);  // inflated vs stack-locked|neutral|biased
  bne_PPC(CCR0, IsInflated);

  if (PrintPreciseRTMLockingStatistics || profile_rtm) {
    Label L_noincrement;
    if (RTMTotalCountIncrRate > 1) {
      branch_on_random_using_tb(tmp, RTMTotalCountIncrRate, L_noincrement);
    }
    assert(stack_rtm_counters != NULL, "should not be NULL when profiling RTM");
    load_const_optimized(tmp, (address)stack_rtm_counters->total_count_addr(), R0);
    //atomic_inc_ptr(tmp, /*temp, will be reloaded*/mark_word); We don't increment atomically
    ldx_PPC(mark_word, tmp);
    addi_PPC(mark_word, mark_word, 1);
    stdx_PPC(mark_word, tmp);
    bind(L_noincrement);
  }
  tbegin__PPC();
  beq_PPC(CCR0, L_on_abort);
  ld_PPC(mark_word, oopDesc::mark_offset_in_bytes(), obj);         // Reload in transaction, conflicts need to be tracked.
  andi_PPC(R0, mark_word, markOopDesc::biased_lock_mask_in_place); // look at 3 lock bits
  cmpwi_PPC(flag, R0, markOopDesc::unlocked_value);                // bits = 001 unlocked
  beq_PPC(flag, DONE_LABEL);                                       // all done if unlocked

  if (UseRTMXendForLockBusy) {
    tend__PPC();
    b_PPC(L_decrement_retry);
  } else {
    tabort__PPC();
  }
  bind(L_on_abort);
  const Register abort_status_Reg = tmp;
  mftexasr_PPC(abort_status_Reg);
  if (PrintPreciseRTMLockingStatistics || profile_rtm) {
    rtm_profiling(abort_status_Reg, /*temp*/mark_word, stack_rtm_counters, method_data, profile_rtm);
  }
  ld_PPC(mark_word, oopDesc::mark_offset_in_bytes(), obj); // reload
  if (RTMRetryCount > 0) {
    // Retry on lock abort if abort status is not permanent.
    rtm_retry_lock_on_abort(retry_on_abort_count_Reg, abort_status_Reg, L_rtm_retry, &L_decrement_retry);
  } else {
    bind(L_decrement_retry);
  }
}

// Use RTM for inflating locks
// inputs: obj       (object to lock)
//         mark_word (current header - KILLED)
//         boxReg    (on-stack box address (displaced header location) - KILLED)
void MacroAssembler::rtm_inflated_locking(ConditionRegister flag,
                                          Register obj, Register mark_word, Register boxReg,
                                          Register retry_on_busy_count_Reg, Register retry_on_abort_count_Reg,
                                          RTMLockingCounters* rtm_counters,
                                          Metadata* method_data, bool profile_rtm,
                                          Label& DONE_LABEL) {
  assert(UseRTMLocking, "why call this otherwise?");
  Label L_rtm_retry, L_decrement_retry, L_on_abort;
  // Clean monitor_value bit to get valid pointer.
  int owner_offset = ObjectMonitor::owner_offset_in_bytes() - markOopDesc::monitor_value;

  // Store non-null, using boxReg instead of (intptr_t)markOopDesc::unused_mark().
  std_PPC(boxReg, BasicLock::displaced_header_offset_in_bytes(), boxReg);
  const Register tmpReg = boxReg;
  const Register owner_addr_Reg = mark_word;
  addi_PPC(owner_addr_Reg, mark_word, owner_offset);

  if (RTMRetryCount > 0) {
    load_const_optimized(retry_on_busy_count_Reg, RTMRetryCount);  // Retry on lock busy.
    load_const_optimized(retry_on_abort_count_Reg, RTMRetryCount); // Retry on abort.
    bind(L_rtm_retry);
  }
  if (PrintPreciseRTMLockingStatistics || profile_rtm) {
    Label L_noincrement;
    if (RTMTotalCountIncrRate > 1) {
      branch_on_random_using_tb(R0, RTMTotalCountIncrRate, L_noincrement);
    }
    assert(rtm_counters != NULL, "should not be NULL when profiling RTM");
    load_const_PPC(R0, (address)rtm_counters->total_count_addr(), tmpReg);
    //atomic_inc_ptr(R0, tmpReg); We don't increment atomically
    ldx_PPC(tmpReg, R0);
    addi_PPC(tmpReg, tmpReg, 1);
    stdx_PPC(tmpReg, R0);
    bind(L_noincrement);
  }
  tbegin__PPC();
  beq_PPC(CCR0, L_on_abort);
  // We don't reload mark word. Will only be reset at safepoint.
  ld_PPC(R0, 0, owner_addr_Reg); // Load in transaction, conflicts need to be tracked.
  cmpdi_PPC(flag, R0, 0);
  beq_PPC(flag, DONE_LABEL);

  if (UseRTMXendForLockBusy) {
    tend__PPC();
    b_PPC(L_decrement_retry);
  } else {
    tabort__PPC();
  }
  bind(L_on_abort);
  const Register abort_status_Reg = tmpReg;
  mftexasr_PPC(abort_status_Reg);
  if (PrintPreciseRTMLockingStatistics || profile_rtm) {
    rtm_profiling(abort_status_Reg, /*temp*/ owner_addr_Reg, rtm_counters, method_data, profile_rtm);
    // Restore owner_addr_Reg
    ld_PPC(mark_word, oopDesc::mark_offset_in_bytes(), obj);
#ifdef ASSERT
//    FIXME_RISCV
//    andi__PPC(R0, mark_word, markOopDesc::monitor_value);
//    asm_assert_ne("must be inflated", 0xa754); // Deflating only allowed at safepoint.
#endif
    addi_PPC(owner_addr_Reg, mark_word, owner_offset);
  }
  if (RTMRetryCount > 0) {
    // Retry on lock abort if abort status is not permanent.
    rtm_retry_lock_on_abort(retry_on_abort_count_Reg, abort_status_Reg, L_rtm_retry);
  }

  // Appears unlocked - try to swing _owner from null to non-null.
  cmpxchgd(flag, /*current val*/ R0, (intptr_t)0, /*new val*/ R24_thread, owner_addr_Reg,
           MacroAssembler::MemBarRel | MacroAssembler::MemBarAcq,
           MacroAssembler::cmpxchgx_hint_acquire_lock(), noreg, &L_decrement_retry, true);

  if (RTMRetryCount > 0) {
    // success done else retry
    b_PPC(DONE_LABEL);
    bind(L_decrement_retry);
    // Spin and retry if lock is busy.
    rtm_retry_lock_on_busy(retry_on_busy_count_Reg, owner_addr_Reg, L_rtm_retry);
  } else {
    bind(L_decrement_retry);
  }
}

#endif //  INCLUDE_RTM_OPT

// "The box" is the space on the stack where we copy the object mark.
void MacroAssembler::compiler_fast_lock_object(ConditionRegister flag, Register oop, Register box,
                                               Register temp, Register displaced_header, Register current_header,
                                               bool try_bias,
                                               RTMLockingCounters* rtm_counters,
                                               RTMLockingCounters* stack_rtm_counters,
                                               Metadata* method_data,
                                               bool use_rtm, bool profile_rtm) {
  assert_different_registers(oop, box, temp, displaced_header, current_header);
  assert(flag != CCR0, "bad condition register");
  Label cont;
  Label object_has_monitor;
  Label cas_failed;

  // Load markOop from object into displaced_header.
  ld_PPC(displaced_header, oopDesc::mark_offset_in_bytes(), oop);


  if (try_bias) {
    biased_locking_enter(flag, oop, displaced_header, temp, current_header, cont);
  }

#if INCLUDE_RTM_OPT
  if (UseRTMForStackLocks && use_rtm) {
    rtm_stack_locking(flag, oop, displaced_header, temp, /*temp*/ current_header,
                      stack_rtm_counters, method_data, profile_rtm,
                      cont, object_has_monitor);
  }
#endif // INCLUDE_RTM_OPT

  // Handle existing monitor.
  // The object has an existing monitor iff (mark & monitor_value) != 0.
  andi__PPC(temp, displaced_header, markOopDesc::monitor_value);
  bne_PPC(CCR0, object_has_monitor);

  // Set displaced_header to be (markOop of object | UNLOCK_VALUE).
  ori_PPC(displaced_header, displaced_header, markOopDesc::unlocked_value);

  // Load Compare Value application register.

  // Initialize the box. (Must happen before we update the object mark!)
  std_PPC(displaced_header, BasicLock::displaced_header_offset_in_bytes(), box);

  // Must fence, otherwise, preceding store(s) may float below cmpxchg.
  // Compare object markOop with mark and if equal exchange scratch1 with object markOop.
  cmpxchgd(/*flag=*/flag,
           /*current_value=*/current_header,
           /*compare_value=*/displaced_header,
           /*exchange_value=*/box,
           /*where=*/oop,
           MacroAssembler::MemBarRel | MacroAssembler::MemBarAcq,
           MacroAssembler::cmpxchgx_hint_acquire_lock(),
           noreg,
           &cas_failed,
           /*check without membar and ldarx first*/true);
  assert(oopDesc::mark_offset_in_bytes() == 0, "offset of _mark is not 0");

  // If the compare-and-exchange succeeded, then we found an unlocked
  // object and we have now locked it.
  b_PPC(cont);

  bind(cas_failed);
  // We did not see an unlocked object so try the fast recursive case.

  // Check if the owner is self by comparing the value in the markOop of object
  // (current_header) with the stack pointer.
  sub_PPC(current_header, current_header, R1_SP_PPC);
  load_const_optimized(temp, ~(os::vm_page_size()-1) | markOopDesc::lock_mask_in_place);

  and__PPC(R0/*==0?*/, current_header, temp);
  // If condition is true we are cont and hence we can store 0 as the
  // displaced header in the box, which indicates that it is a recursive lock.
  mcrf_PPC(flag,CCR0);
  std_PPC(R0/*==0, perhaps*/, BasicLock::displaced_header_offset_in_bytes(), box);

  // Handle existing monitor.
  b_PPC(cont);

  bind(object_has_monitor);
  // The object's monitor m is unlocked iff m->owner == NULL,
  // otherwise m->owner may contain a thread or a stack address.

#if INCLUDE_RTM_OPT
  // Use the same RTM locking code in 32- and 64-bit VM.
  if (use_rtm) {
    rtm_inflated_locking(flag, oop, displaced_header, box, temp, /*temp*/ current_header,
                         rtm_counters, method_data, profile_rtm, cont);
  } else {
#endif // INCLUDE_RTM_OPT

  // Try to CAS m->owner from NULL to current thread.
  addi_PPC(temp, displaced_header, ObjectMonitor::owner_offset_in_bytes()-markOopDesc::monitor_value);
  cmpxchgd(/*flag=*/flag,
           /*current_value=*/current_header,
           /*compare_value=*/(intptr_t)0,
           /*exchange_value=*/R24_thread,
           /*where=*/temp,
           MacroAssembler::MemBarRel | MacroAssembler::MemBarAcq,
           MacroAssembler::cmpxchgx_hint_acquire_lock());

  // Store a non-null value into the box.
  std_PPC(box, BasicLock::displaced_header_offset_in_bytes(), box);

# ifdef ASSERT
  bne_PPC(flag, cont);
  // We have acquired the monitor, check some invariants.
  addi_PPC(/*monitor=*/temp, temp, -ObjectMonitor::owner_offset_in_bytes());
  // Invariant 1: _recursions should be 0.
  //assert(ObjectMonitor::recursions_size_in_bytes() == 8, "unexpected size");
  asm_assert_mem8_is_zero(ObjectMonitor::recursions_offset_in_bytes(), temp,
                            "monitor->_recursions should be 0", -1);
# endif

#if INCLUDE_RTM_OPT
  } // use_rtm()
#endif

  bind(cont);
  // flag == EQ indicates success
  // flag == NE indicates failure
}

void MacroAssembler::compiler_fast_unlock_object(ConditionRegister flag, Register oop, Register box,
                                                 Register temp, Register displaced_header, Register current_header,
                                                 bool try_bias, bool use_rtm) {
  assert_different_registers(oop, box, temp, displaced_header, current_header);
  assert(flag != CCR0, "bad condition register");
  Label cont;
  Label object_has_monitor;

  if (try_bias) {
    biased_locking_exit(flag, oop, current_header, cont);
  }

#if INCLUDE_RTM_OPT
  if (UseRTMForStackLocks && use_rtm) {
    assert(!UseBiasedLocking, "Biased locking is not supported with RTM locking");
    Label L_regular_unlock;
    ld_PPC(current_header, oopDesc::mark_offset_in_bytes(), oop);         // fetch markword
    andi_PPC(R0, current_header, markOopDesc::biased_lock_mask_in_place); // look at 3 lock bits
    cmpwi_PPC(flag, R0, markOopDesc::unlocked_value);                     // bits = 001 unlocked
    bne_PPC(flag, L_regular_unlock);                                      // else RegularLock
    tend__PPC();                                                          // otherwise end...
    b_PPC(cont);                                                          // ... and we're done
    bind(L_regular_unlock);
  }
#endif

  // Find the lock address and load the displaced header from the stack.
  ld_PPC(displaced_header, BasicLock::displaced_header_offset_in_bytes(), box);

  // If the displaced header is 0, we have a recursive unlock.
  cmpdi_PPC(flag, displaced_header, 0);
  beq_PPC(flag, cont);

  // Handle existing monitor.
  // The object has an existing monitor iff (mark & monitor_value) != 0.
  RTM_OPT_ONLY( if (!(UseRTMForStackLocks && use_rtm)) ) // skip load if already done
  ld_PPC(current_header, oopDesc::mark_offset_in_bytes(), oop);
  andi__PPC(R0, current_header, markOopDesc::monitor_value);
  bne_PPC(CCR0, object_has_monitor);

  // Check if it is still a light weight lock, this is is true if we see
  // the stack address of the basicLock in the markOop of the object.
  // Cmpxchg sets flag to cmpd_PPC(current_header, box).
  cmpxchgd(/*flag=*/flag,
           /*current_value=*/current_header,
           /*compare_value=*/box,
           /*exchange_value=*/displaced_header,
           /*where=*/oop,
           MacroAssembler::MemBarRel,
           MacroAssembler::cmpxchgx_hint_release_lock(),
           noreg,
           &cont);

  assert(oopDesc::mark_offset_in_bytes() == 0, "offset of _mark is not 0");

  // Handle existing monitor.
  b_PPC(cont);

  bind(object_has_monitor);
  addi_PPC(current_header, current_header, -markOopDesc::monitor_value); // monitor
  ld_PPC(temp,             ObjectMonitor::owner_offset_in_bytes(), current_header);

    // It's inflated.
#if INCLUDE_RTM_OPT
  if (use_rtm) {
    Label L_regular_inflated_unlock;
    // Clean monitor_value bit to get valid pointer
    cmpdi_PPC(flag, temp, 0);
    bne_PPC(flag, L_regular_inflated_unlock);
    tend__PPC();
    b_PPC(cont);
    bind(L_regular_inflated_unlock);
  }
#endif

  ld_PPC(displaced_header, ObjectMonitor::recursions_offset_in_bytes(), current_header);
  xorr_PPC(temp, R24_thread, temp);      // Will be 0 if we are the owner.
  orr_PPC(temp, temp, displaced_header); // Will be 0 if there are 0 recursions.
  cmpdi_PPC(flag, temp, 0);
  bne_PPC(flag, cont);

  ld_PPC(temp,             ObjectMonitor::EntryList_offset_in_bytes(), current_header);
  ld_PPC(displaced_header, ObjectMonitor::cxq_offset_in_bytes(), current_header);
  orr_PPC(temp, temp, displaced_header); // Will be 0 if both are 0.
  cmpdi_PPC(flag, temp, 0);
  bne_PPC(flag, cont);
  release();
  std_PPC(temp, ObjectMonitor::owner_offset_in_bytes(), current_header);

  bind(cont);
  // flag == EQ indicates success
  // flag == NE indicates failure
}

void MacroAssembler::safepoint_poll(Label& slow_path, Register temp_reg) {
  if (SafepointMechanism::uses_thread_local_poll()) {
    // Armed page has poll_bit set.
    ld(temp_reg, R24_thread, in_bytes(Thread::polling_page_offset()));
  } else {
    int off = load_const_optimized(temp_reg, SafepointSynchronize::address_of_state());
    lwu(temp_reg, temp_reg, off);
  }
  assert(SafepointSynchronize::_not_synchronized == 0, "Compare with zero should means this value");
  bnez(temp_reg, slow_path);
}

void MacroAssembler::resolve_jobject(Register value, Register tmp1, Register tmp2, bool needs_frame) {
  BarrierSetAssembler* bs = BarrierSet::barrier_set()->barrier_set_assembler();
  bs->resolve_jobject(this, value, tmp1, tmp2, needs_frame);
}

// Values for last_Java_pc, and last_Java_sp must comply to the rules
// in frame_riscv.hpp.
void MacroAssembler::set_last_Java_frame(Register last_Java_sp, Register last_Java_fp, Register last_Java_pc) {
  // Always set last_Java_pc and flags first because once last_Java_sp
  // is visible has_last_Java_frame is true and users will look at the
  // rest of the fields. (Note: flags should always be zero before we
  // get here so doesn't need to be set.)

  // Verify that last_Java_pc was zeroed on return to Java
  asm_assert_mem8_is_zero(in_bytes(JavaThread::last_Java_pc_offset()), R24_thread,
                          "last_Java_pc not zeroed before leaving Java", 0x200);

  // When returning from calling out from Java mode the frame anchor's
  // last_Java_pc will always be set to NULL. It is set here so that
  // if we are doing a call to native (not VM) that we capture the
  // known pc and don't have to rely on the native call having a
  // standard frame linkage where we can find the pc.
  if (last_Java_pc != noreg)
    sd(last_Java_pc, R24_thread, in_bytes(JavaThread::last_Java_pc_offset()));

  if (last_Java_fp != noreg)
    sd(last_Java_fp, R24_thread, in_bytes(JavaThread::last_Java_fp_offset()));

//  sd(last_Java_fp, R24_thread, in_bytes(JavaThread::last_Java_fp_offset()));

  // Set last_Java_sp last.
  sd(last_Java_sp, R24_thread, in_bytes(JavaThread::last_Java_sp_offset()));
}

void MacroAssembler::reset_last_Java_frame() {
  asm_assert_mem8_isnot_zero(in_bytes(JavaThread::last_Java_sp_offset()),
                             R24_thread, "SP was not set, still zero", 0x202);

  BLOCK_COMMENT("reset_last_Java_frame {");
  // _last_Java_sp = 0
  sd(R0_ZERO, R24_thread, in_bytes(JavaThread::last_Java_sp_offset()));

  // _last_Java_sp = 0
  sd(R0_ZERO, R24_thread, in_bytes(JavaThread::last_Java_fp_offset()));

  // _last_Java_pc = 0
  sd(R0_ZERO, R24_thread, in_bytes(JavaThread::last_Java_pc_offset()));
  BLOCK_COMMENT("} reset_last_Java_frame");
}

void MacroAssembler::set_top_ijava_frame_at_SP_as_last_Java_frame(Register sp, Register fp, Register tmp1) {
  assert_different_registers(sp, tmp1);

  // sp points to a TOP_IJAVA_FRAME, retrieve frame's PC via
  // TOP_IJAVA_FRAME_ABI.
  // FIXME: assert that we really have a TOP_IJAVA_FRAME here!
  address entry = pc();


  li(tmp1, entry);

  set_last_Java_frame(/*sp=*/sp, fp, /*pc=*/tmp1);
}

void MacroAssembler::set_top_ijava_frame_at_SP_as_last_Java_frame_2(Register sp, Register fp, Register tmp1) {
  assert_different_registers(sp, tmp1);

  // sp points to a TOP_IJAVA_FRAME, retrieve frame's PC via
  // TOP_IJAVA_FRAME_ABI.
  // FIXME: assert that we really have a TOP_IJAVA_FRAME here!
  address entry = pc();

  li(tmp1, /*entry*/ entry);

  set_last_Java_frame(/*sp=*/sp, fp, /*pc=*/tmp1);
}


void MacroAssembler::get_vm_result(Register oop_result) {
  // Read:
  //   R24_thread
  //   R24_thread->in_bytes(JavaThread::vm_result_offset())
  //
  // Updated:
  //   oop_result
  //   R24_thread->in_bytes(JavaThread::vm_result_offset())

  verify_thread();

  ld(oop_result, R24_thread, in_bytes(JavaThread::vm_result_offset()));
  sd(R0_ZERO, R24_thread, in_bytes(JavaThread::vm_result_offset()));

  verify_oop(oop_result);
}

void MacroAssembler::get_vm_result_2(Register metadata_result) {
  // Read:
  //   R24_thread
  //   R24_thread->in_bytes(JavaThread::vm_result_2_offset())
  //
  // Updated:
  //   metadata_result
  //   R24_thread->in_bytes(JavaThread::vm_result_2_offset())

  ld(metadata_result, R24_thread, in_bytes(JavaThread::vm_result_2_offset()));
  sd(R0_ZERO, R24_thread, in_bytes(JavaThread::vm_result_2_offset()));
}

Register MacroAssembler::encode_klass_not_null(Register dst, Register src) {
  Register current = (src != noreg) ? src : dst; // Klass is in dst if no src provided.
  if (CompressedKlassPointers::base() != 0) {
    // Use dst as temp if it is free.
    sub_const_optimized(dst, current, CompressedKlassPointers::base(), R0);
    current = dst;
  }
  if (CompressedKlassPointers::shift() != 0) {
    srdi_PPC(dst, current, CompressedKlassPointers::shift());
    current = dst;
  }
  return current;
}

void MacroAssembler::store_klass(Register dst_oop, Register klass, Register ck) {
  if (UseCompressedClassPointers) {
    Register compressedKlass = encode_klass_not_null(ck, klass);
    stw_PPC(compressedKlass, oopDesc::klass_offset_in_bytes(), dst_oop);
  } else {
    std_PPC(klass, oopDesc::klass_offset_in_bytes(), dst_oop);
  }
}

void MacroAssembler::store_klass_gap(Register dst_oop, Register val) {
  if (UseCompressedClassPointers) {
    if (val == noreg) {
      val = R0;
      li_PPC(val, 0);
    }
    stw_PPC(val, oopDesc::klass_gap_offset_in_bytes(), dst_oop); // klass gap if compressed
  }
}

int MacroAssembler::instr_size_for_decode_klass_not_null() {
  if (!UseCompressedClassPointers) return 0;
  int num_instrs = 1;  // shift or move
  if (CompressedKlassPointers::base() != 0) num_instrs = 7;  // shift + load const + add
  return num_instrs * BytesPerInstWord;
}

void MacroAssembler::decode_klass_not_null(Register dst, Register src) {
  assert(dst != R0, "Dst reg may not be R0, as R0 is used here.");
  if (src == noreg) src = dst;
  Register shifted_src = src;
  if (CompressedKlassPointers::shift() != 0 ||
      CompressedKlassPointers::base() == 0 && src != dst) {  // Move required.
    shifted_src = dst;
    slli(shifted_src, src, CompressedKlassPointers::shift());
  }
  if (CompressedKlassPointers::base() != 0) {
    add_const_optimized(dst, shifted_src, CompressedKlassPointers::base(), R0);
  }
}

void MacroAssembler::load_klass(Register dst, Register src) {
  // TODO_RISCV: add support for compressed class pointers
  if (UseCompressedClassPointers) {
    lwu(dst, src, oopDesc::klass_offset_in_bytes());
    // Attention: no null check here!
    decode_klass_not_null(dst, dst);
  } else {
    ld(dst, src, oopDesc::klass_offset_in_bytes());
  }
}

// ((OopHandle)result).resolve();
void MacroAssembler::resolve_oop_handle(Register result) {
  // OopHandle::resolve is an indirection.
  ld(result, result, 0);
}

void MacroAssembler::load_mirror_from_const_method(Register mirror, Register const_method) {
  ld(mirror, const_method, in_bytes(ConstMethod::constants_offset()));
  ld(mirror, mirror, ConstantPool::pool_holder_offset_in_bytes());
  ld(mirror, mirror, in_bytes(Klass::java_mirror_offset()));
  resolve_oop_handle(mirror);
}

void MacroAssembler::load_method_holder(Register holder, Register method) {
  ld(holder, method, in_bytes(Method::const_offset()));
  ld(holder, holder, in_bytes(ConstMethod::constants_offset()));
  ld(holder, holder, ConstantPool::pool_holder_offset_in_bytes());
}

// Clear Array
// For very short arrays. tmp == R0 is allowed.
void MacroAssembler::clear_memory_unrolled(Register base_ptr, int cnt_dwords, Register tmp, int offset) {
  if (cnt_dwords > 0) { li_PPC(tmp, 0); }
  for (int i = 0; i < cnt_dwords; ++i) { std_PPC(tmp, offset + i * 8, base_ptr); }
}

// Version for constant short array length. Kills base_ptr. tmp == R0 is allowed.
void MacroAssembler::clear_memory_constlen(Register base_ptr, int cnt_dwords, Register tmp) {
  if (cnt_dwords < 8) {
    clear_memory_unrolled(base_ptr, cnt_dwords, tmp);
    return;
  }

  Label loop;
  const long loopcnt   = cnt_dwords >> 1,
             remainder = cnt_dwords & 1;

  li_PPC(tmp, loopcnt);
  mtctr_PPC(tmp);
  li_PPC(tmp, 0);
  bind(loop);
    std_PPC(tmp, 0, base_ptr);
    std_PPC(tmp, 8, base_ptr);
    addi_PPC(base_ptr, base_ptr, 16);
    bdnz_PPC(loop);
  if (remainder) { std_PPC(tmp, 0, base_ptr); }
}

// Kills both input registers. tmp == R0 is allowed.
void MacroAssembler::clear_memory_doubleword(Register base_ptr, Register cnt_dwords, Register tmp, long const_cnt) {
  // Procedure for large arrays (uses data cache block zero instruction).
    Label startloop, fast, fastloop, small_rest, restloop, done;
    const int cl_size         = VM_Version::L1_data_cache_line_size(),
              cl_dwords       = cl_size >> 3,
              cl_dw_addr_bits = exact_log2(cl_dwords),
              dcbz_min        = 1,  // Min count of dcbz executions, needs to be >0.
              min_cnt         = ((dcbz_min + 1) << cl_dw_addr_bits) - 1;

  if (const_cnt >= 0) {
    // Constant case.
    if (const_cnt < min_cnt) {
      clear_memory_constlen(base_ptr, const_cnt, tmp);
      return;
    }
    load_const_optimized(cnt_dwords, const_cnt, tmp);
  } else {
    // cnt_dwords already loaded in register. Need to check size.
    cmpdi_PPC(CCR1, cnt_dwords, min_cnt); // Big enough? (ensure >= dcbz_min lines included).
    blt_PPC(CCR1, small_rest);
  }
    rldicl__PPC(tmp, base_ptr, 64-3, 64-cl_dw_addr_bits); // Extract dword offset within first cache line.
    beq_PPC(CCR0, fast);                                  // Already 128byte aligned.

    subfic_PPC(tmp, tmp, cl_dwords);
    mtctr_PPC(tmp);                        // Set ctr to hit 128byte boundary (0<ctr<cl_dwords).
    subf_PPC(cnt_dwords, tmp, cnt_dwords); // rest.
    li_PPC(tmp, 0);

  bind(startloop);                     // Clear at the beginning to reach 128byte boundary.
    std_PPC(tmp, 0, base_ptr);             // Clear 8byte aligned block.
    addi_PPC(base_ptr, base_ptr, 8);
    bdnz_PPC(startloop);

  bind(fast);                                  // Clear 128byte blocks.
    srdi_PPC(tmp, cnt_dwords, cl_dw_addr_bits);    // Loop count for 128byte loop (>0).
    andi_PPC(cnt_dwords, cnt_dwords, cl_dwords-1); // Rest in dwords.
    mtctr_PPC(tmp);                                // Load counter.

  bind(fastloop);
    dcbz_PPC(base_ptr);                    // Clear 128byte aligned block.
    addi_PPC(base_ptr, base_ptr, cl_size);
    bdnz_PPC(fastloop);

  bind(small_rest);
    cmpdi_PPC(CCR0, cnt_dwords, 0);        // size 0?
    beq_PPC(CCR0, done);                   // rest == 0
    li_PPC(tmp, 0);
    mtctr_PPC(cnt_dwords);                 // Load counter.

  bind(restloop);                      // Clear rest.
    std_PPC(tmp, 0, base_ptr);             // Clear 8byte aligned block.
    addi_PPC(base_ptr, base_ptr, 8);
    bdnz_PPC(restloop);

  bind(done);
}

/////////////////////////////////////////// String intrinsics ////////////////////////////////////////////

#ifdef COMPILER2
// Intrinsics for CompactStrings

// Compress char[] to byte[] by compressing 16 bytes at once.
void MacroAssembler::string_compress_16(Register src, Register dst, Register cnt,
                                        Register tmp1, Register tmp2, Register tmp3, Register tmp4, Register tmp5,
                                        Label& Lfailure) {

  const Register tmp0 = R0;
  assert_different_registers(src, dst, cnt, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5);
  Label Lloop, Lslow;

  // Check if cnt >= 8 (= 16 bytes)
  lis_PPC(tmp1, 0xFF);                // tmp1 = 0x00FF00FF00FF00FF
  srwi__PPC(tmp2, cnt, 3);
  beq_PPC(CCR0, Lslow);
  ori_PPC(tmp1, tmp1, 0xFF);
  rldimi_PPC(tmp1, tmp1, 32, 0);
  mtctr_PPC(tmp2);

  // 2x unrolled loop
  bind(Lloop);
  ld_PPC(tmp2, 0, src);               // _0_1_2_3 (Big Endian)
  ld_PPC(tmp4, 8, src);               // _4_5_6_7

  orr_PPC(tmp0, tmp2, tmp4);
  rldicl_PPC(tmp3, tmp2, 6*8, 64-24); // _____1_2
  rldimi_PPC(tmp2, tmp2, 2*8, 2*8);   // _0_2_3_3
  rldicl_PPC(tmp5, tmp4, 6*8, 64-24); // _____5_6
  rldimi_PPC(tmp4, tmp4, 2*8, 2*8);   // _4_6_7_7

  andc__PPC(tmp0, tmp0, tmp1);
  bne_PPC(CCR0, Lfailure);            // Not latin1.
  addi_PPC(src, src, 16);

  rlwimi_PPC(tmp3, tmp2, 0*8, 24, 31);// _____1_3
  srdi_PPC(tmp2, tmp2, 3*8);          // ____0_2_
  rlwimi_PPC(tmp5, tmp4, 0*8, 24, 31);// _____5_7
  srdi_PPC(tmp4, tmp4, 3*8);          // ____4_6_

  orr_PPC(tmp2, tmp2, tmp3);          // ____0123
  orr_PPC(tmp4, tmp4, tmp5);          // ____4567

  stw_PPC(tmp2, 0, dst);
  stw_PPC(tmp4, 4, dst);
  addi_PPC(dst, dst, 8);
  bdnz_PPC(Lloop);

  bind(Lslow);                    // Fallback to slow version
}

// Compress char[] to byte[]. cnt must be positive int.
void MacroAssembler::string_compress(Register src, Register dst, Register cnt, Register tmp, Label& Lfailure) {
  Label Lloop;
  mtctr_PPC(cnt);

  bind(Lloop);
  lhz_PPC(tmp, 0, src);
  cmplwi_PPC(CCR0, tmp, 0xff);
  bgt_PPC(CCR0, Lfailure);            // Not latin1.
  addi_PPC(src, src, 2);
  stb_PPC(tmp, 0, dst);
  addi_PPC(dst, dst, 1);
  bdnz_PPC(Lloop);
}

// Inflate byte[] to char[] by inflating 16 bytes at once.
void MacroAssembler::string_inflate_16(Register src, Register dst, Register cnt,
                                       Register tmp1, Register tmp2, Register tmp3, Register tmp4, Register tmp5) {
  const Register tmp0 = R0;
  assert_different_registers(src, dst, cnt, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5);
  Label Lloop, Lslow;

  // Check if cnt >= 8
  srwi__PPC(tmp2, cnt, 3);
  beq_PPC(CCR0, Lslow);
  lis_PPC(tmp1, 0xFF);                // tmp1 = 0x00FF00FF
  ori_PPC(tmp1, tmp1, 0xFF);
  mtctr_PPC(tmp2);

  // 2x unrolled loop
  bind(Lloop);
  lwz_PPC(tmp2, 0, src);              // ____0123 (Big Endian)
  lwz_PPC(tmp4, 4, src);              // ____4567
  addi_PPC(src, src, 8);

  rldicl_PPC(tmp3, tmp2, 7*8, 64-8);  // _______2
  rlwimi_PPC(tmp2, tmp2, 3*8, 16, 23);// ____0113
  rldicl_PPC(tmp5, tmp4, 7*8, 64-8);  // _______6
  rlwimi_PPC(tmp4, tmp4, 3*8, 16, 23);// ____4557

  andc_PPC(tmp0, tmp2, tmp1);         // ____0_1_
  rlwimi_PPC(tmp2, tmp3, 2*8, 0, 23); // _____2_3
  andc_PPC(tmp3, tmp4, tmp1);         // ____4_5_
  rlwimi_PPC(tmp4, tmp5, 2*8, 0, 23); // _____6_7

  rldimi_PPC(tmp2, tmp0, 3*8, 0*8);   // _0_1_2_3
  rldimi_PPC(tmp4, tmp3, 3*8, 0*8);   // _4_5_6_7

  std_PPC(tmp2, 0, dst);
  std_PPC(tmp4, 8, dst);
  addi_PPC(dst, dst, 16);
  bdnz_PPC(Lloop);

  bind(Lslow);                    // Fallback to slow version
}

// Inflate byte[] to char[]. cnt must be positive int.
void MacroAssembler::string_inflate(Register src, Register dst, Register cnt, Register tmp) {
  Label Lloop;
  mtctr_PPC(cnt);

  bind(Lloop);
  lbz_PPC(tmp, 0, src);
  addi_PPC(src, src, 1);
  sth_PPC(tmp, 0, dst);
  addi_PPC(dst, dst, 2);
  bdnz_PPC(Lloop);
}

void MacroAssembler::string_compare(Register str1, Register str2,
                                    Register cnt1, Register cnt2,
                                    Register tmp1, Register result, int ae) {
  const Register tmp0 = R0,
                 diff = tmp1;

  assert_different_registers(str1, str2, cnt1, cnt2, tmp0, tmp1, result);
  Label Ldone, Lslow, Lloop, Lreturn_diff;

  // Note: Making use of the fact that compareTo(a, b) == -compareTo(b, a)
  // we interchange str1 and str2 in the UL case and negate the result.
  // Like this, str1 is always latin1 encoded, except for the UU case.
  // In addition, we need 0 (or sign which is 0) extend.

  if (ae == StrIntrinsicNode::UU) {
    srwi_PPC(cnt1, cnt1, 1);
  } else {
    clrldi_PPC(cnt1, cnt1, 32);
  }

  if (ae != StrIntrinsicNode::LL) {
    srwi_PPC(cnt2, cnt2, 1);
  } else {
    clrldi_PPC(cnt2, cnt2, 32);
  }

  // See if the lengths are different, and calculate min in cnt1.
  // Save diff in case we need it for a tie-breaker.
  subf__PPC(diff, cnt2, cnt1); // diff = cnt1 - cnt2
  // if (diff > 0) { cnt1 = cnt2; }
  if (VM_Version::has_isel()) {
    isel_PPC(cnt1, CCR0, Assembler::greater, /*invert*/ false, cnt2);
  } else {
    Label Lskip;
    blt_PPC(CCR0, Lskip);
    mr_PPC(cnt1, cnt2);
    bind(Lskip);
  }

  // Rename registers
  Register chr1 = result;
  Register chr2 = tmp0;

  // Compare multiple characters in fast loop (only implemented for same encoding).
  int stride1 = 8, stride2 = 8;
  if (ae == StrIntrinsicNode::LL || ae == StrIntrinsicNode::UU) {
    int log2_chars_per_iter = (ae == StrIntrinsicNode::LL) ? 3 : 2;
    Label Lfastloop, Lskipfast;

    srwi__PPC(tmp0, cnt1, log2_chars_per_iter);
    beq_PPC(CCR0, Lskipfast);
    rldicl_PPC(cnt2, cnt1, 0, 64 - log2_chars_per_iter); // Remaining characters.
    li_PPC(cnt1, 1 << log2_chars_per_iter); // Initialize for failure case: Rescan characters from current iteration.
    mtctr_PPC(tmp0);

    bind(Lfastloop);
    ld_PPC(chr1, 0, str1);
    ld_PPC(chr2, 0, str2);
    cmpd_PPC(CCR0, chr1, chr2);
    bne_PPC(CCR0, Lslow);
    addi_PPC(str1, str1, stride1);
    addi_PPC(str2, str2, stride2);
    bdnz_PPC(Lfastloop);
    mr_PPC(cnt1, cnt2); // Remaining characters.
    bind(Lskipfast);
  }

  // Loop which searches the first difference character by character.
  cmpwi_PPC(CCR0, cnt1, 0);
  beq_PPC(CCR0, Lreturn_diff);
  bind(Lslow);
  mtctr_PPC(cnt1);

  switch (ae) {
    case StrIntrinsicNode::LL: stride1 = 1; stride2 = 1; break;
    case StrIntrinsicNode::UL: // fallthru (see comment above)
    case StrIntrinsicNode::LU: stride1 = 1; stride2 = 2; break;
    case StrIntrinsicNode::UU: stride1 = 2; stride2 = 2; break;
    default: ShouldNotReachHere(); break;
  }

  bind(Lloop);
  if (stride1 == 1) { lbz_PPC(chr1, 0, str1); } else { lhz_PPC(chr1, 0, str1); }
  if (stride2 == 1) { lbz_PPC(chr2, 0, str2); } else { lhz_PPC(chr2, 0, str2); }
  subf__PPC(result, chr2, chr1); // result = chr1 - chr2
  bne_PPC(CCR0, Ldone);
  addi_PPC(str1, str1, stride1);
  addi_PPC(str2, str2, stride2);
  bdnz_PPC(Lloop);

  // If strings are equal up to min length, return the length difference.
  bind(Lreturn_diff);
  mr_PPC(result, diff);

  // Otherwise, return the difference between the first mismatched chars.
  bind(Ldone);
  if (ae == StrIntrinsicNode::UL) {
    neg_PPC(result, result); // Negate result (see note above).
  }
}

void MacroAssembler::array_equals(bool is_array_equ, Register ary1, Register ary2,
                                  Register limit, Register tmp1, Register result, bool is_byte) {
  const Register tmp0 = R0;
  assert_different_registers(ary1, ary2, limit, tmp0, tmp1, result);
  Label Ldone, Lskiploop, Lloop, Lfastloop, Lskipfast;
  bool limit_needs_shift = false;

  if (is_array_equ) {
    const int length_offset = arrayOopDesc::length_offset_in_bytes();
    const int base_offset   = arrayOopDesc::base_offset_in_bytes(is_byte ? T_BYTE : T_CHAR);

    // Return true if the same array.
    cmpd_PPC(CCR0, ary1, ary2);
    beq_PPC(CCR0, Lskiploop);

    // Return false if one of them is NULL.
    cmpdi_PPC(CCR0, ary1, 0);
    cmpdi_PPC(CCR1, ary2, 0);
    li_PPC(result, 0);
    cror_PPC(CCR0, Assembler::equal, CCR1, Assembler::equal);
    beq_PPC(CCR0, Ldone);

    // Load the lengths of arrays.
    lwz_PPC(limit, length_offset, ary1);
    lwz_PPC(tmp0, length_offset, ary2);

    // Return false if the two arrays are not equal length.
    cmpw_PPC(CCR0, limit, tmp0);
    bne_PPC(CCR0, Ldone);

    // Load array addresses.
    addi_PPC(ary1, ary1, base_offset);
    addi_PPC(ary2, ary2, base_offset);
  } else {
    limit_needs_shift = !is_byte;
    li_PPC(result, 0); // Assume not equal.
  }

  // Rename registers
  Register chr1 = tmp0;
  Register chr2 = tmp1;

  // Compare 8 bytes per iteration in fast loop.
  const int log2_chars_per_iter = is_byte ? 3 : 2;

  srwi__PPC(tmp0, limit, log2_chars_per_iter + (limit_needs_shift ? 1 : 0));
  beq_PPC(CCR0, Lskipfast);
  mtctr_PPC(tmp0);

  bind(Lfastloop);
  ld_PPC(chr1, 0, ary1);
  ld_PPC(chr2, 0, ary2);
  addi_PPC(ary1, ary1, 8);
  addi_PPC(ary2, ary2, 8);
  cmpd_PPC(CCR0, chr1, chr2);
  bne_PPC(CCR0, Ldone);
  bdnz_PPC(Lfastloop);

  bind(Lskipfast);
  rldicl__PPC(limit, limit, limit_needs_shift ? 64 - 1 : 0, 64 - log2_chars_per_iter); // Remaining characters.
  beq_PPC(CCR0, Lskiploop);
  mtctr_PPC(limit);

  // Character by character.
  bind(Lloop);
  if (is_byte) {
    lbz_PPC(chr1, 0, ary1);
    lbz_PPC(chr2, 0, ary2);
    addi_PPC(ary1, ary1, 1);
    addi_PPC(ary2, ary2, 1);
  } else {
    lhz_PPC(chr1, 0, ary1);
    lhz_PPC(chr2, 0, ary2);
    addi_PPC(ary1, ary1, 2);
    addi_PPC(ary2, ary2, 2);
  }
  cmpw_PPC(CCR0, chr1, chr2);
  bne_PPC(CCR0, Ldone);
  bdnz_PPC(Lloop);

  bind(Lskiploop);
  li_PPC(result, 1); // All characters are equal.
  bind(Ldone);
}

void MacroAssembler::string_indexof(Register result, Register haystack, Register haycnt,
                                    Register needle, ciTypeArray* needle_values, Register needlecnt, int needlecntval,
                                    Register tmp1, Register tmp2, Register tmp3, Register tmp4, int ae) {

  // Ensure 0<needlecnt<=haycnt in ideal graph as prerequisite!
  Label L_TooShort, L_Found, L_NotFound, L_End;
  Register last_addr = haycnt, // Kill haycnt at the beginning.
  addr      = tmp1,
  n_start   = tmp2,
  ch1       = tmp3,
  ch2       = R0;

  assert(ae != StrIntrinsicNode::LU, "Invalid encoding");
  const int h_csize = (ae == StrIntrinsicNode::LL) ? 1 : 2;
  const int n_csize = (ae == StrIntrinsicNode::UU) ? 2 : 1;

  // **************************************************************************************************
  // Prepare for main loop: optimized for needle count >=2, bail out otherwise.
  // **************************************************************************************************

  // Compute last haystack addr to use if no match gets found.
  clrldi_PPC(haycnt, haycnt, 32);         // Ensure positive int is valid as 64 bit value.
  addi_PPC(addr, haystack, -h_csize);     // Accesses use pre-increment.
  if (needlecntval == 0) { // variable needlecnt
   cmpwi_PPC(CCR6, needlecnt, 2);
   clrldi_PPC(needlecnt, needlecnt, 32);  // Ensure positive int is valid as 64 bit value.
   blt_PPC(CCR6, L_TooShort);             // Variable needlecnt: handle short needle separately.
  }

  if (n_csize == 2) { lwz_PPC(n_start, 0, needle); } else { lhz_PPC(n_start, 0, needle); } // Load first 2 characters of needle.

  if (needlecntval == 0) { // variable needlecnt
   subf_PPC(ch1, needlecnt, haycnt);      // Last character index to compare is haycnt-needlecnt.
   addi_PPC(needlecnt, needlecnt, -2);    // Rest of needle.
  } else { // constant needlecnt
  guarantee(needlecntval != 1, "IndexOf with single-character needle must be handled separately");
  assert((needlecntval & 0x7fff) == needlecntval, "wrong immediate");
   addi_PPC(ch1, haycnt, -needlecntval);  // Last character index to compare is haycnt-needlecnt.
   if (needlecntval > 3) { li_PPC(needlecnt, needlecntval - 2); } // Rest of needle.
  }

  if (h_csize == 2) { slwi_PPC(ch1, ch1, 1); } // Scale to number of bytes.

  if (ae ==StrIntrinsicNode::UL) {
   srwi_PPC(tmp4, n_start, 1*8);          // ___0
   rlwimi_PPC(n_start, tmp4, 2*8, 0, 23); // _0_1
  }

  add_PPC(last_addr, haystack, ch1);      // Point to last address to compare (haystack+2*(haycnt-needlecnt)).

  // Main Loop (now we have at least 2 characters).
  Label L_OuterLoop, L_InnerLoop, L_FinalCheck, L_Comp1, L_Comp2;
  bind(L_OuterLoop); // Search for 1st 2 characters.
  Register addr_diff = tmp4;
   subf_PPC(addr_diff, addr, last_addr);  // Difference between already checked address and last address to check.
   addi_PPC(addr, addr, h_csize);         // This is the new address we want to use for comparing.
   srdi__PPC(ch2, addr_diff, h_csize);
   beq_PPC(CCR0, L_FinalCheck);           // 2 characters left?
   mtctr_PPC(ch2);                        // num of characters / 2
  bind(L_InnerLoop);                  // Main work horse (2x unrolled search loop)
   if (h_csize == 2) {                // Load 2 characters of haystack (ignore alignment).
    lwz_PPC(ch1, 0, addr);
    lwz_PPC(ch2, 2, addr);
   } else {
    lhz_PPC(ch1, 0, addr);
    lhz_PPC(ch2, 1, addr);
   }
   cmpw_PPC(CCR0, ch1, n_start);          // Compare 2 characters (1 would be sufficient but try to reduce branches to CompLoop).
   cmpw_PPC(CCR1, ch2, n_start);
   beq_PPC(CCR0, L_Comp1);                // Did we find the needle start?
   beq_PPC(CCR1, L_Comp2);
   addi_PPC(addr, addr, 2 * h_csize);
   bdnz_PPC(L_InnerLoop);
  bind(L_FinalCheck);
   andi__PPC(addr_diff, addr_diff, h_csize); // Remaining characters not covered by InnerLoop: (num of characters) & 1.
   beq_PPC(CCR0, L_NotFound);
   if (h_csize == 2) { lwz_PPC(ch1, 0, addr); } else { lhz_PPC(ch1, 0, addr); } // One position left at which we have to compare.
   cmpw_PPC(CCR1, ch1, n_start);
   beq_PPC(CCR1, L_Comp1);
  bind(L_NotFound);
   li_PPC(result, -1);                    // not found
   b_PPC(L_End);

   // **************************************************************************************************
   // Special Case: unfortunately, the variable needle case can be called with needlecnt<2
   // **************************************************************************************************
  if (needlecntval == 0) {           // We have to handle these cases separately.
  Label L_OneCharLoop;
  bind(L_TooShort);
   mtctr_PPC(haycnt);
   if (n_csize == 2) { lhz_PPC(n_start, 0, needle); } else { lbz_PPC(n_start, 0, needle); } // First character of needle
  bind(L_OneCharLoop);
   if (h_csize == 2) { lhzu_PPC(ch1, 2, addr); } else { lbzu_PPC(ch1, 1, addr); }
   cmpw_PPC(CCR1, ch1, n_start);
   beq_PPC(CCR1, L_Found);               // Did we find the one character needle?
   bdnz_PPC(L_OneCharLoop);
   li_PPC(result, -1);                   // Not found.
   b_PPC(L_End);
  }

  // **************************************************************************************************
  // Regular Case Part II: compare rest of needle (first 2 characters have been compared already)
  // **************************************************************************************************

  // Compare the rest
  bind(L_Comp2);
   addi_PPC(addr, addr, h_csize);        // First comparison has failed, 2nd one hit.
  bind(L_Comp1);                     // Addr points to possible needle start.
  if (needlecntval != 2) {           // Const needlecnt==2?
   if (needlecntval != 3) {
    if (needlecntval == 0) { beq_PPC(CCR6, L_Found); } // Variable needlecnt==2?
    Register n_ind = tmp4,
             h_ind = n_ind;
    li_PPC(n_ind, 2 * n_csize);          // First 2 characters are already compared, use index 2.
    mtctr_PPC(needlecnt);                // Decremented by 2, still > 0.
   Label L_CompLoop;
   bind(L_CompLoop);
    if (ae ==StrIntrinsicNode::UL) {
      h_ind = ch1;
      sldi_PPC(h_ind, n_ind, 1);
    }
    if (n_csize == 2) { lhzx_PPC(ch2, needle, n_ind); } else { lbzx_PPC(ch2, needle, n_ind); }
    if (h_csize == 2) { lhzx_PPC(ch1, addr, h_ind); } else { lbzx_PPC(ch1, addr, h_ind); }
    cmpw_PPC(CCR1, ch1, ch2);
    bne_PPC(CCR1, L_OuterLoop);
    addi_PPC(n_ind, n_ind, n_csize);
    bdnz_PPC(L_CompLoop);
   } else { // No loop required if there's only one needle character left.
    if (n_csize == 2) { lhz_PPC(ch2, 2 * 2, needle); } else { lbz_PPC(ch2, 2 * 1, needle); }
    if (h_csize == 2) { lhz_PPC(ch1, 2 * 2, addr); } else { lbz_PPC(ch1, 2 * 1, addr); }
    cmpw_PPC(CCR1, ch1, ch2);
    bne_PPC(CCR1, L_OuterLoop);
   }
  }
  // Return index ...
  bind(L_Found);
   subf_PPC(result, haystack, addr);     // relative to haystack, ...
   if (h_csize == 2) { srdi_PPC(result, result, 1); } // in characters.
  bind(L_End);
} // string_indexof

void MacroAssembler::string_indexof_char(Register result, Register haystack, Register haycnt,
                                         Register needle, jchar needleChar, Register tmp1, Register tmp2, bool is_byte) {
  assert_different_registers(haystack, haycnt, needle, tmp1, tmp2);

  Label L_InnerLoop, L_FinalCheck, L_Found1, L_Found2, L_NotFound, L_End;
  Register addr = tmp1,
           ch1 = tmp2,
           ch2 = R0;

  const int h_csize = is_byte ? 1 : 2;

//4:
   srwi__PPC(tmp2, haycnt, 1);   // Shift right by exact_log2(UNROLL_FACTOR).
   mr_PPC(addr, haystack);
   beq_PPC(CCR0, L_FinalCheck);
   mtctr_PPC(tmp2);              // Move to count register.
//8:
  bind(L_InnerLoop);         // Main work horse (2x unrolled search loop).
   if (!is_byte) {
    lhz_PPC(ch1, 0, addr);
    lhz_PPC(ch2, 2, addr);
   } else {
    lbz_PPC(ch1, 0, addr);
    lbz_PPC(ch2, 1, addr);
   }
   (needle != R0) ? cmpw_PPC(CCR0, ch1, needle) : cmplwi_PPC(CCR0, ch1, (unsigned int)needleChar);
   (needle != R0) ? cmpw_PPC(CCR1, ch2, needle) : cmplwi_PPC(CCR1, ch2, (unsigned int)needleChar);
   beq_PPC(CCR0, L_Found1);      // Did we find the needle?
   beq_PPC(CCR1, L_Found2);
   addi_PPC(addr, addr, 2 * h_csize);
   bdnz_PPC(L_InnerLoop);
//16:
  bind(L_FinalCheck);
   andi__PPC(R0, haycnt, 1);
   beq_PPC(CCR0, L_NotFound);
   if (!is_byte) { lhz_PPC(ch1, 0, addr); } else { lbz_PPC(ch1, 0, addr); } // One position left at which we have to compare.
   (needle != R0) ? cmpw_PPC(CCR1, ch1, needle) : cmplwi_PPC(CCR1, ch1, (unsigned int)needleChar);
   beq_PPC(CCR1, L_Found1);
//21:
  bind(L_NotFound);
   li_PPC(result, -1);           // Not found.
   b_PPC(L_End);

  bind(L_Found2);
   addi_PPC(addr, addr, h_csize);
//24:
  bind(L_Found1);            // Return index ...
   subf_PPC(result, haystack, addr); // relative to haystack, ...
   if (!is_byte) { srdi_PPC(result, result, 1); } // in characters.
  bind(L_End);
} // string_indexof_char


void MacroAssembler::has_negatives(Register src, Register cnt, Register result,
                                   Register tmp1, Register tmp2) {
  const Register tmp0 = R0;
  assert_different_registers(src, result, cnt, tmp0, tmp1, tmp2);
  Label Lfastloop, Lslow, Lloop, Lnoneg, Ldone;

  // Check if cnt >= 8 (= 16 bytes)
  lis_PPC(tmp1, (int)(short)0x8080);  // tmp1 = 0x8080808080808080
  srwi__PPC(tmp2, cnt, 4);
  li_PPC(result, 1);                  // Assume there's a negative byte.
  beq_PPC(CCR0, Lslow);
  ori_PPC(tmp1, tmp1, 0x8080);
  rldimi_PPC(tmp1, tmp1, 32, 0);
  mtctr_PPC(tmp2);

  // 2x unrolled loop
  bind(Lfastloop);
  ld_PPC(tmp2, 0, src);
  ld_PPC(tmp0, 8, src);

  orr_PPC(tmp0, tmp2, tmp0);

  and__PPC(tmp0, tmp0, tmp1);
  bne_PPC(CCR0, Ldone);               // Found negative byte.
  addi_PPC(src, src, 16);

  bdnz_PPC(Lfastloop);

  bind(Lslow);                    // Fallback to slow version
  rldicl__PPC(tmp0, cnt, 0, 64-4);
  beq_PPC(CCR0, Lnoneg);
  mtctr_PPC(tmp0);
  bind(Lloop);
  lbz_PPC(tmp0, 0, src);
  addi_PPC(src, src, 1);
  andi__PPC(tmp0, tmp0, 0x80);
  bne_PPC(CCR0, Ldone);               // Found negative byte.
  bdnz_PPC(Lloop);
  bind(Lnoneg);
  li_PPC(result, 0);

  bind(Ldone);
}

#endif // Compiler2

// Helpers for Intrinsic Emitters
//
// Revert the byte order of a 32bit value in a register
//   src: 0x44556677
//   dst: 0x77665544
// Three steps to obtain the result:
//  1) Rotate src (as doubleword) left 5 bytes. That puts the leftmost byte of the src word
//     into the rightmost byte position. Afterwards, everything left of the rightmost byte is cleared.
//     This value initializes dst.
//  2) Rotate src (as word) left 3 bytes. That puts the rightmost byte of the src word into the leftmost
//     byte position. Furthermore, byte 5 is rotated into byte 6 position where it is supposed to go.
//     This value is mask inserted into dst with a [0..23] mask of 1s.
//  3) Rotate src (as word) left 1 byte. That puts byte 6 into byte 5 position.
//     This value is mask inserted into dst with a [8..15] mask of 1s.
void MacroAssembler::load_reverse_32(Register dst, Register src) {
  assert_different_registers(dst, src);

  rldicl_PPC(dst, src, (4+1)*8, 56);       // Rotate byte 4 into position 7 (rightmost), clear all to the left.
  rlwimi_PPC(dst, src,     3*8,  0, 23);   // Insert byte 5 into position 6, 7 into 4, leave pos 7 alone.
  rlwimi_PPC(dst, src,     1*8,  8, 15);   // Insert byte 6 into position 5, leave the rest alone.
}

// Calculate the column addresses of the crc32 lookup table into distinct registers.
// This loop-invariant calculation is moved out of the loop body, reducing the loop
// body size from 20 to 16 instructions.
// Returns the offset that was used to calculate the address of column tc3.
// Due to register shortage, setting tc3 may overwrite table. With the return offset
// at hand, the original table address can be easily reconstructed.
int MacroAssembler::crc32_table_columns(Register table, Register tc0, Register tc1, Register tc2, Register tc3) {
  assert(!VM_Version::has_vpmsumb(), "Vector version should be used instead!");

  // Point to 4 byte folding tables (byte-reversed version for Big Endian)
  // Layout: See StubRoutines::generate_crc_constants.
#ifdef VM_LITTLE_ENDIAN
  const int ix0 = 3 * CRC32_TABLE_SIZE;
  const int ix1 = 2 * CRC32_TABLE_SIZE;
  const int ix2 = 1 * CRC32_TABLE_SIZE;
  const int ix3 = 0 * CRC32_TABLE_SIZE;
#else
  const int ix0 = 1 * CRC32_TABLE_SIZE;
  const int ix1 = 2 * CRC32_TABLE_SIZE;
  const int ix2 = 3 * CRC32_TABLE_SIZE;
  const int ix3 = 4 * CRC32_TABLE_SIZE;
#endif
  assert_different_registers(table, tc0, tc1, tc2);
  assert(table == tc3, "must be!");

  addi_PPC(tc0, table, ix0);
  addi_PPC(tc1, table, ix1);
  addi_PPC(tc2, table, ix2);
  if (ix3 != 0) addi_PPC(tc3, table, ix3);

  return ix3;
}

/**
 * uint32_t crc;
 * table[crc & 0xFF] ^ (crc >> 8);
 */
void MacroAssembler::fold_byte_crc32(Register crc, Register val, Register table, Register tmp) {
  assert_different_registers(crc, table, tmp);
  assert_different_registers(val, table);

  if (crc == val) {                   // Must rotate first to use the unmodified value.
    rlwinm_PPC(tmp, val, 2, 24-2, 31-2);  // Insert (rightmost) byte 7 of val, shifted left by 2, into byte 6..7 of tmp, clear the rest.
                                      // As we use a word (4-byte) instruction, we have to adapt the mask bit positions.
    srwi_PPC(crc, crc, 8);                // Unsigned shift, clear leftmost 8 bits.
  } else {
    srwi_PPC(crc, crc, 8);                // Unsigned shift, clear leftmost 8 bits.
    rlwinm_PPC(tmp, val, 2, 24-2, 31-2);  // Insert (rightmost) byte 7 of val, shifted left by 2, into byte 6..7 of tmp, clear the rest.
  }
  lwzx_PPC(tmp, table, tmp);
  xorr_PPC(crc, crc, tmp);
}

/**
 * Emits code to update CRC-32 with a byte value according to constants in table.
 *
 * @param [in,out]crc   Register containing the crc.
 * @param [in]val       Register containing the byte to fold into the CRC.
 * @param [in]table     Register containing the table of crc constants.
 *
 * uint32_t crc;
 * val = crc_table[(val ^ crc) & 0xFF];
 * crc = val ^ (crc >> 8);
 */
void MacroAssembler::update_byte_crc32(Register crc, Register val, Register table) {
  BLOCK_COMMENT("update_byte_crc32:");
  xorr_PPC(val, val, crc);
  fold_byte_crc32(crc, val, table, val);
}

/**
 * @param crc   register containing existing CRC (32-bit)
 * @param buf   register pointing to input byte buffer (byte*)
 * @param len   register containing number of bytes
 * @param table register pointing to CRC table
 */
void MacroAssembler::update_byteLoop_crc32(Register crc, Register buf, Register len, Register table,
                                           Register data, bool loopAlignment) {
  assert_different_registers(crc, buf, len, table, data);

  Label L_mainLoop, L_done;
  const int mainLoop_stepping  = 1;
  const int mainLoop_alignment = loopAlignment ? 32 : 4; // (InputForNewCode > 4 ? InputForNewCode : 32) : 4;

  // Process all bytes in a single-byte loop.
  clrldi__PPC(len, len, 32);                         // Enforce 32 bit. Anything to do?
  beq_PPC(CCR0, L_done);

  mtctr_PPC(len);
  align(mainLoop_alignment);
  BIND(L_mainLoop);
    lbz_PPC(data, 0, buf);                           // Byte from buffer, zero-extended.
    addi_PPC(buf, buf, mainLoop_stepping);           // Advance buffer position.
    update_byte_crc32(crc, data, table);
    bdnz_PPC(L_mainLoop);                            // Iterate.

  bind(L_done);
}

/**
 * Emits code to update CRC-32 with a 4-byte value according to constants in table
 * Implementation according to jdk/src/share/native/java/util/zip/zlib-1.2.8/crc32.c
 */
// A note on the lookup table address(es):
// The implementation uses 4 table columns (byte-reversed versions for Big Endian).
// To save the effort of adding the column offset to the table address each time
// a table element is looked up, it is possible to pass the pre-calculated
// column addresses.
// Uses R9..R12 as work register. Must be saved/restored by caller, if necessary.
void MacroAssembler::update_1word_crc32(Register crc, Register buf, Register table, int bufDisp, int bufInc,
                                        Register t0,  Register t1,  Register t2,  Register t3,
                                        Register tc0, Register tc1, Register tc2, Register tc3) {
  assert_different_registers(crc, t3);

  // XOR crc with next four bytes of buffer.
  lwz_PPC(t3, bufDisp, buf);
  if (bufInc != 0) {
    addi_PPC(buf, buf, bufInc);
  }
  xorr_PPC(t3, t3, crc);

  // Chop crc into 4 single-byte pieces, shifted left 2 bits, to form the table indices.
  rlwinm_PPC(t0, t3,  2,         24-2, 31-2);  // ((t1 >>  0) & 0xff) << 2
  rlwinm_PPC(t1, t3,  32+(2- 8), 24-2, 31-2);  // ((t1 >>  8) & 0xff) << 2
  rlwinm_PPC(t2, t3,  32+(2-16), 24-2, 31-2);  // ((t1 >> 16) & 0xff) << 2
  rlwinm_PPC(t3, t3,  32+(2-24), 24-2, 31-2);  // ((t1 >> 24) & 0xff) << 2

  // Use the pre-calculated column addresses.
  // Load pre-calculated table values.
  lwzx_PPC(t0, tc0, t0);
  lwzx_PPC(t1, tc1, t1);
  lwzx_PPC(t2, tc2, t2);
  lwzx_PPC(t3, tc3, t3);

  // Calculate new crc from table values.
  xorr_PPC(t0,  t0, t1);
  xorr_PPC(t2,  t2, t3);
  xorr_PPC(crc, t0, t2);  // Now crc contains the final checksum value.
}

/**
 * @param crc   register containing existing CRC (32-bit)
 * @param buf   register pointing to input byte buffer (byte*)
 * @param len   register containing number of bytes
 * @param table register pointing to CRC table
 *
 * uses R9..R12 as work register. Must be saved/restored by caller!
 */
void MacroAssembler::kernel_crc32_1word(Register crc, Register buf, Register len, Register table,
                                        Register t0,  Register t1,  Register t2,  Register t3,
                                        Register tc0, Register tc1, Register tc2, Register tc3,
                                        bool invertCRC) {
  assert_different_registers(crc, buf, len, table);

  Label L_mainLoop, L_tail;
  Register  tmp          = t0;
  Register  data         = t0;
  Register  tmp2         = t1;
  const int mainLoop_stepping  = 4;
  const int tailLoop_stepping  = 1;
  const int log_stepping       = exact_log2(mainLoop_stepping);
  const int mainLoop_alignment = 32; // InputForNewCode > 4 ? InputForNewCode : 32;
  const int complexThreshold   = 2*mainLoop_stepping;

  // Don't test for len <= 0 here. This pathological case should not occur anyway.
  // Optimizing for it by adding a test and a branch seems to be a waste of CPU cycles
  // for all well-behaved cases. The situation itself is detected and handled correctly
  // within update_byteLoop_crc32.
  assert(tailLoop_stepping == 1, "check tailLoop_stepping!");

  BLOCK_COMMENT("kernel_crc32_1word {");

  if (invertCRC) {
    nand_PPC(crc, crc, crc);                      // 1s complement of crc
  }

  // Check for short (<mainLoop_stepping) buffer.
  cmpdi_PPC(CCR0, len, complexThreshold);
  blt_PPC(CCR0, L_tail);

  // Pre-mainLoop alignment did show a slight (1%) positive effect on performance.
  // We leave the code in for reference. Maybe we need alignment when we exploit vector instructions.
  {
    // Align buf addr to mainLoop_stepping boundary.
    neg_PPC(tmp2, buf);                              // Calculate # preLoop iterations for alignment.
    rldicl_PPC(tmp2, tmp2, 0, 64-log_stepping);      // Rotate tmp2 0 bits, insert into tmp2, anding with mask with 1s from 62..63.

    if (complexThreshold > mainLoop_stepping) {
      sub_PPC(len, len, tmp2);                       // Remaining bytes for main loop (>=mainLoop_stepping is guaranteed).
    } else {
      sub_PPC(tmp, len, tmp2);                       // Remaining bytes for main loop.
      cmpdi_PPC(CCR0, tmp, mainLoop_stepping);
      blt_PPC(CCR0, L_tail);                         // For less than one mainloop_stepping left, do only tail processing
      mr_PPC(len, tmp);                              // remaining bytes for main loop (>=mainLoop_stepping is guaranteed).
    }
    update_byteLoop_crc32(crc, buf, tmp2, table, data, false);
  }

  srdi_PPC(tmp2, len, log_stepping);                 // #iterations for mainLoop
  andi_PPC(len, len, mainLoop_stepping-1);           // remaining bytes for tailLoop
  mtctr_PPC(tmp2);

#ifdef VM_LITTLE_ENDIAN
  Register crc_rv = crc;
#else
  Register crc_rv = tmp;                         // Load_reverse needs separate registers to work on.
                                                 // Occupies tmp, but frees up crc.
  load_reverse_32(crc_rv, crc);                  // Revert byte order because we are dealing with big-endian data.
  tmp = crc;
#endif

  int reconstructTableOffset = crc32_table_columns(table, tc0, tc1, tc2, tc3);

  align(mainLoop_alignment);                     // Octoword-aligned loop address. Shows 2% improvement.
  BIND(L_mainLoop);
    update_1word_crc32(crc_rv, buf, table, 0, mainLoop_stepping, crc_rv, t1, t2, t3, tc0, tc1, tc2, tc3);
    bdnz_PPC(L_mainLoop);

#ifndef VM_LITTLE_ENDIAN
  load_reverse_32(crc, crc_rv);                  // Revert byte order because we are dealing with big-endian data.
  tmp = crc_rv;                                  // Tmp uses it's original register again.
#endif

  // Restore original table address for tailLoop.
  if (reconstructTableOffset != 0) {
    addi_PPC(table, table, -reconstructTableOffset);
  }

  // Process last few (<complexThreshold) bytes of buffer.
  BIND(L_tail);
  update_byteLoop_crc32(crc, buf, len, table, data, false);

  if (invertCRC) {
    nand_PPC(crc, crc, crc);                      // 1s complement of crc
  }
  BLOCK_COMMENT("} kernel_crc32_1word");
}

/**
 * @param crc             register containing existing CRC (32-bit)
 * @param buf             register pointing to input byte buffer (byte*)
 * @param len             register containing number of bytes
 * @param constants       register pointing to precomputed constants
 * @param t0-t6           temp registers
 */
void MacroAssembler::kernel_crc32_vpmsum(Register crc, Register buf, Register len, Register constants,
                                         Register t0, Register t1, Register t2, Register t3,
                                         Register t4, Register t5, Register t6, bool invertCRC) {
  assert_different_registers(crc, buf, len, constants);

  Label L_tail;

  BLOCK_COMMENT("kernel_crc32_vpmsum {");

  if (invertCRC) {
    nand_PPC(crc, crc, crc);                      // 1s complement of crc
  }

  // Enforce 32 bit.
  clrldi_PPC(len, len, 32);

  // Align if we have enough bytes for the fast version.
  const int alignment = 16,
            threshold = 32;
  Register prealign = t0;

  neg_PPC(prealign, buf);
  addi_PPC(t1, len, -threshold);
  andi_PPC(prealign, prealign, alignment - 1);
  cmpw_PPC(CCR0, t1, prealign);
  blt_PPC(CCR0, L_tail); // len - prealign < threshold?

  subf_PPC(len, prealign, len);
  update_byteLoop_crc32(crc, buf, prealign, constants, t2, false);

  // Calculate from first aligned address as far as possible.
  addi_PPC(constants, constants, CRC32_TABLE_SIZE); // Point to vector constants.
  kernel_crc32_vpmsum_aligned(crc, buf, len, constants, t0, t1, t2, t3, t4, t5, t6);
  addi_PPC(constants, constants, -CRC32_TABLE_SIZE); // Point to table again.

  // Remaining bytes.
  BIND(L_tail);
  update_byteLoop_crc32(crc, buf, len, constants, t2, false);

  if (invertCRC) {
    nand_PPC(crc, crc, crc);                      // 1s complement of crc
  }

  BLOCK_COMMENT("} kernel_crc32_vpmsum");
}

/**
 * @param crc             register containing existing CRC (32-bit)
 * @param buf             register pointing to input byte buffer (byte*)
 * @param len             register containing number of bytes (will get updated to remaining bytes)
 * @param constants       register pointing to CRC table for 128-bit aligned memory
 * @param t0-t6           temp registers
 */
void MacroAssembler::kernel_crc32_vpmsum_aligned(Register crc, Register buf, Register len, Register constants,
    Register t0, Register t1, Register t2, Register t3, Register t4, Register t5, Register t6) {

  // Save non-volatile vector registers (frameless).
  Register offset = t1;
  int offsetInt = 0;
  offsetInt -= 16; li_PPC(offset, offsetInt); stvx_PPC(VR20, offset, R1_SP_PPC);
  offsetInt -= 16; li_PPC(offset, offsetInt); stvx_PPC(VR21, offset, R1_SP_PPC);
  offsetInt -= 16; li_PPC(offset, offsetInt); stvx_PPC(VR22, offset, R1_SP_PPC);
  offsetInt -= 16; li_PPC(offset, offsetInt); stvx_PPC(VR23, offset, R1_SP_PPC);
  offsetInt -= 16; li_PPC(offset, offsetInt); stvx_PPC(VR24, offset, R1_SP_PPC);
  offsetInt -= 16; li_PPC(offset, offsetInt); stvx_PPC(VR25, offset, R1_SP_PPC);
#ifndef VM_LITTLE_ENDIAN
  offsetInt -= 16; li_PPC(offset, offsetInt); stvx_PPC(VR26, offset, R1_SP_PPC);
#endif
  offsetInt -= 8; std_PPC(R14, offsetInt, R1_SP_PPC);
  offsetInt -= 8; std_PPC(R15, offsetInt, R1_SP_PPC);

  // Implementation uses an inner loop which uses between 256 and 16 * unroll_factor
  // bytes per iteration. The basic scheme is:
  // lvx: load vector (Big Endian needs reversal)
  // vpmsumw: carry-less 32 bit multiplications with constant representing a large CRC shift
  // vxor: xor partial results together to get unroll_factor2 vectors

  // Outer loop performs the CRC shifts needed to combine the unroll_factor2 vectors.

  // Using 16 * unroll_factor / unroll_factor_2 bytes for constants.
  const int unroll_factor = CRC32_UNROLL_FACTOR,
            unroll_factor2 = CRC32_UNROLL_FACTOR2;

  const int outer_consts_size = (unroll_factor2 - 1) * 16,
            inner_consts_size = (unroll_factor / unroll_factor2) * 16;

  // Support registers.
  Register offs[] = { noreg, t0, t1, t2, t3, t4, t5, t6 };
  Register num_bytes = R14,
           loop_count = R15,
           cur_const = crc; // will live in VCRC
  // Constant array for outer loop: unroll_factor2 - 1 registers,
  // Constant array for inner loop: unroll_factor / unroll_factor2 registers.
  VectorRegister consts0[] = { VR16, VR17, VR18, VR19, VR20, VR21, VR22 },
                 consts1[] = { VR23, VR24 };
  // Data register arrays: 2 arrays with unroll_factor2 registers.
  VectorRegister data0[] = { VR0, VR1, VR2, VR3, VR4, VR5, VR6, VR7 },
                 data1[] = { VR8, VR9, VR10, VR11, VR12, VR13, VR14, VR15 };

  VectorRegister VCRC = data0[0];
  VectorRegister Vc = VR25;
  VectorRegister swap_bytes = VR26; // Only for Big Endian.

  // We have at least 1 iteration (ensured by caller).
  Label L_outer_loop, L_inner_loop, L_last;

  // If supported set DSCR pre-fetch to deepest.
  if (VM_Version::has_mfdscr()) {
    load_const_optimized(t0, VM_Version::_dscr_val | 7);
    mtdscr_PPC(t0);
  }

  mtvrwz_PPC(VCRC, crc); // crc lives in VCRC, now

  for (int i = 1; i < unroll_factor2; ++i) {
    li_PPC(offs[i], 16 * i);
  }

  // Load consts for outer loop
  lvx_PPC(consts0[0], constants);
  for (int i = 1; i < unroll_factor2 - 1; ++i) {
    lvx_PPC(consts0[i], offs[i], constants);
  }

  load_const_optimized(num_bytes, 16 * unroll_factor);

  // Reuse data registers outside of the loop.
  VectorRegister Vtmp = data1[0];
  VectorRegister Vtmp2 = data1[1];
  VectorRegister zeroes = data1[2];

  vspltisb_PPC(Vtmp, 0);
  vsldoi_PPC(VCRC, Vtmp, VCRC, 8); // 96 bit zeroes, 32 bit CRC.

  // Load vector for vpermxor_PPC (to xor both 64 bit parts together)
  lvsl_PPC(Vtmp, buf);   // 000102030405060708090a0b0c0d0e0f
  vspltisb_PPC(Vc, 4);
  vsl_PPC(Vc, Vtmp, Vc); // 00102030405060708090a0b0c0d0e0f0
  xxspltd_PPC(Vc->to_vsr(), Vc->to_vsr(), 0);
  vor_PPC(Vc, Vtmp, Vc); // 001122334455667708192a3b4c5d6e7f

#ifdef VM_LITTLE_ENDIAN
#define BE_swap_bytes(x)
#else
  vspltisb_PPC(Vtmp2, 0xf);
  vxor_PPC(swap_bytes, Vtmp, Vtmp2);
#define BE_swap_bytes(x) vperm_PPC(x, x, x, swap_bytes)
#endif

  cmpd_PPC(CCR0, len, num_bytes);
  blt_PPC(CCR0, L_last);

  addi_PPC(cur_const, constants, outer_consts_size); // Point to consts for inner loop
  load_const_optimized(loop_count, unroll_factor / (2 * unroll_factor2) - 1); // One double-iteration peeled off.

  // ********** Main loop start **********
  align(32);
  bind(L_outer_loop);

  // Begin of unrolled first iteration (no xor).
  lvx_PPC(data1[0], buf);
  for (int i = 1; i < unroll_factor2 / 2; ++i) {
    lvx_PPC(data1[i], offs[i], buf);
  }
  vpermxor_PPC(VCRC, VCRC, VCRC, Vc); // xor both halves to 64 bit result.
  lvx_PPC(consts1[0], cur_const);
  mtctr_PPC(loop_count);
  for (int i = 0; i < unroll_factor2 / 2; ++i) {
    BE_swap_bytes(data1[i]);
    if (i == 0) { vxor_PPC(data1[0], data1[0], VCRC); } // xor in previous CRC.
    lvx_PPC(data1[i + unroll_factor2 / 2], offs[i + unroll_factor2 / 2], buf);
    vpmsumw_PPC(data0[i], data1[i], consts1[0]);
  }
  addi_PPC(buf, buf, 16 * unroll_factor2);
  subf_PPC(len, num_bytes, len);
  lvx_PPC(consts1[1], offs[1], cur_const);
  addi_PPC(cur_const, cur_const, 32);
  // Begin of unrolled second iteration (head).
  for (int i = 0; i < unroll_factor2 / 2; ++i) {
    BE_swap_bytes(data1[i + unroll_factor2 / 2]);
    if (i == 0) { lvx_PPC(data1[0], buf); } else { lvx_PPC(data1[i], offs[i], buf); }
    vpmsumw_PPC(data0[i + unroll_factor2 / 2], data1[i + unroll_factor2 / 2], consts1[0]);
  }
  for (int i = 0; i < unroll_factor2 / 2; ++i) {
    BE_swap_bytes(data1[i]);
    lvx_PPC(data1[i + unroll_factor2 / 2], offs[i + unroll_factor2 / 2], buf);
    vpmsumw_PPC(data1[i], data1[i], consts1[1]);
  }
  addi_PPC(buf, buf, 16 * unroll_factor2);

  // Generate most performance relevant code. Loads + half of the vpmsumw have been generated.
  // Double-iteration allows using the 2 constant registers alternatingly.
  align(32);
  bind(L_inner_loop);
  for (int j = 1; j < 3; ++j) { // j < unroll_factor / unroll_factor2 - 1 for complete unrolling.
    if (j & 1) {
      lvx_PPC(consts1[0], cur_const);
    } else {
      lvx_PPC(consts1[1], offs[1], cur_const);
      addi_PPC(cur_const, cur_const, 32);
    }
    for (int i = 0; i < unroll_factor2; ++i) {
      int idx = i + unroll_factor2 / 2, inc = 0; // For modulo-scheduled input.
      if (idx >= unroll_factor2) { idx -= unroll_factor2; inc = 1; }
      BE_swap_bytes(data1[idx]);
      vxor_PPC(data0[i], data0[i], data1[i]);
      if (i == 0) lvx_PPC(data1[0], buf); else lvx_PPC(data1[i], offs[i], buf);
      vpmsumw_PPC(data1[idx], data1[idx], consts1[(j + inc) & 1]);
    }
    addi_PPC(buf, buf, 16 * unroll_factor2);
  }
  bdnz_PPC(L_inner_loop);

  addi_PPC(cur_const, constants, outer_consts_size); // Reset

  // Tail of last iteration (no loads).
  for (int i = 0; i < unroll_factor2 / 2; ++i) {
    BE_swap_bytes(data1[i + unroll_factor2 / 2]);
    vxor_PPC(data0[i], data0[i], data1[i]);
    vpmsumw_PPC(data1[i + unroll_factor2 / 2], data1[i + unroll_factor2 / 2], consts1[1]);
  }
  for (int i = 0; i < unroll_factor2 / 2; ++i) {
    vpmsumw_PPC(data0[i], data0[i], consts0[unroll_factor2 - 2 - i]); // First half of fixup shifts.
    vxor_PPC(data0[i + unroll_factor2 / 2], data0[i + unroll_factor2 / 2], data1[i + unroll_factor2 / 2]);
  }

  // Last data register is ok, other ones need fixup shift.
  for (int i = unroll_factor2 / 2; i < unroll_factor2 - 1; ++i) {
    vpmsumw_PPC(data0[i], data0[i], consts0[unroll_factor2 - 2 - i]);
  }

  // Combine to 128 bit result vector VCRC = data0[0].
  for (int i = 1; i < unroll_factor2; i<<=1) {
    for (int j = 0; j <= unroll_factor2 - 2*i; j+=2*i) {
      vxor_PPC(data0[j], data0[j], data0[j+i]);
    }
  }
  cmpd_PPC(CCR0, len, num_bytes);
  bge_PPC(CCR0, L_outer_loop);

  // Last chance with lower num_bytes.
  bind(L_last);
  srdi_PPC(loop_count, len, exact_log2(16 * 2 * unroll_factor2)); // Use double-iterations.
  // Point behind last const for inner loop.
  add_const_optimized(cur_const, constants, outer_consts_size + inner_consts_size);
  sldi_PPC(R0, loop_count, exact_log2(16 * 2)); // Bytes of constants to be used.
  clrrdi_PPC(num_bytes, len, exact_log2(16 * 2 * unroll_factor2));
  subf_PPC(cur_const, R0, cur_const); // Point to constant to be used first.

  addic__PPC(loop_count, loop_count, -1); // One double-iteration peeled off.
  bgt_PPC(CCR0, L_outer_loop);
  // ********** Main loop end **********

  // Restore DSCR pre-fetch value.
  if (VM_Version::has_mfdscr()) {
    load_const_optimized(t0, VM_Version::_dscr_val);
    mtdscr_PPC(t0);
  }

  // ********** Simple loop for remaining 16 byte blocks **********
  {
    Label L_loop, L_done;

    srdi__PPC(t0, len, 4); // 16 bytes per iteration
    clrldi_PPC(len, len, 64-4);
    beq_PPC(CCR0, L_done);

    // Point to const (same as last const for inner loop).
    add_const_optimized(cur_const, constants, outer_consts_size + inner_consts_size - 16);
    mtctr_PPC(t0);
    lvx_PPC(Vtmp2, cur_const);

    align(32);
    bind(L_loop);

    lvx_PPC(Vtmp, buf);
    addi_PPC(buf, buf, 16);
    vpermxor_PPC(VCRC, VCRC, VCRC, Vc); // xor both halves to 64 bit result.
    BE_swap_bytes(Vtmp);
    vxor_PPC(VCRC, VCRC, Vtmp);
    vpmsumw_PPC(VCRC, VCRC, Vtmp2);
    bdnz_PPC(L_loop);

    bind(L_done);
  }
  // ********** Simple loop end **********
#undef BE_swap_bytes

  // Point to Barrett constants
  add_const_optimized(cur_const, constants, outer_consts_size + inner_consts_size);

  vspltisb_PPC(zeroes, 0);

  // Combine to 64 bit result.
  vpermxor_PPC(VCRC, VCRC, VCRC, Vc); // xor both halves to 64 bit result.

  // Reduce to 32 bit CRC: Remainder by multiply-high.
  lvx_PPC(Vtmp, cur_const);
  vsldoi_PPC(Vtmp2, zeroes, VCRC, 12);  // Extract high 32 bit.
  vpmsumd_PPC(Vtmp2, Vtmp2, Vtmp);      // Multiply by inverse long poly.
  vsldoi_PPC(Vtmp2, zeroes, Vtmp2, 12); // Extract high 32 bit.
  vsldoi_PPC(Vtmp, zeroes, Vtmp, 8);
  vpmsumd_PPC(Vtmp2, Vtmp2, Vtmp);      // Multiply quotient by long poly.
  vxor_PPC(VCRC, VCRC, Vtmp2);          // Remainder fits into 32 bit.

  // Move result. len is already updated.
  vsldoi_PPC(VCRC, VCRC, zeroes, 8);
  mfvrd_PPC(crc, VCRC);

  // Restore non-volatile Vector registers (frameless).
  offsetInt = 0;
  offsetInt -= 16; li_PPC(offset, offsetInt); lvx_PPC(VR20, offset, R1_SP_PPC);
  offsetInt -= 16; li_PPC(offset, offsetInt); lvx_PPC(VR21, offset, R1_SP_PPC);
  offsetInt -= 16; li_PPC(offset, offsetInt); lvx_PPC(VR22, offset, R1_SP_PPC);
  offsetInt -= 16; li_PPC(offset, offsetInt); lvx_PPC(VR23, offset, R1_SP_PPC);
  offsetInt -= 16; li_PPC(offset, offsetInt); lvx_PPC(VR24, offset, R1_SP_PPC);
  offsetInt -= 16; li_PPC(offset, offsetInt); lvx_PPC(VR25, offset, R1_SP_PPC);
#ifndef VM_LITTLE_ENDIAN
  offsetInt -= 16; li_PPC(offset, offsetInt); lvx_PPC(VR26, offset, R1_SP_PPC);
#endif
  offsetInt -= 8;  ld_PPC(R14, offsetInt, R1_SP_PPC);
  offsetInt -= 8;  ld_PPC(R15, offsetInt, R1_SP_PPC);
}

void MacroAssembler::crc32(Register crc, Register buf, Register len, Register t0, Register t1, Register t2,
                           Register t3, Register t4, Register t5, Register t6, Register t7, bool is_crc32c) {
  load_const_optimized(t0, is_crc32c ? StubRoutines::crc32c_table_addr()
                                     : StubRoutines::crc_table_addr()   , R0);

  if (VM_Version::has_vpmsumb()) {
    kernel_crc32_vpmsum(crc, buf, len, t0, t1, t2, t3, t4, t5, t6, t7, !is_crc32c);
  } else {
    kernel_crc32_1word(crc, buf, len, t0, t1, t2, t3, t4, t5, t6, t7, t0, !is_crc32c);
  }
}

void MacroAssembler::kernel_crc32_singleByteReg(Register crc, Register val, Register table, bool invertCRC) {
  assert_different_registers(crc, val, table);

  BLOCK_COMMENT("kernel_crc32_singleByteReg:");
  if (invertCRC) {
    nand_PPC(crc, crc, crc);                // 1s complement of crc
  }

  update_byte_crc32(crc, val, table);

  if (invertCRC) {
    nand_PPC(crc, crc, crc);                // 1s complement of crc
  }
}

// dest_lo += src1 + src2
// dest_hi += carry1 + carry2
void MacroAssembler::add2_with_carry(Register dest_hi,
                                     Register dest_lo,
                                     Register src1, Register src2) {
  li_PPC(R0, 0);
  addc_PPC(dest_lo, dest_lo, src1);
  adde_PPC(dest_hi, dest_hi, R0);
  addc_PPC(dest_lo, dest_lo, src2);
  adde_PPC(dest_hi, dest_hi, R0);
}

// Multiply 64 bit by 64 bit first loop.
void MacroAssembler::multiply_64_x_64_loop(Register x, Register xstart,
                                           Register x_xstart,
                                           Register y, Register y_idx,
                                           Register z,
                                           Register carry,
                                           Register product_high, Register product,
                                           Register idx, Register kdx,
                                           Register tmp) {
  //  jlong carry, x[], y[], z[];
  //  for (int idx=ystart, kdx=ystart+1+xstart; idx >= 0; idx--, kdx--) {
  //    huge_128 product = y[idx] * x[xstart] + carry;
  //    z[kdx] = (jlong)product;
  //    carry  = (jlong)(product >>> 64);
  //  }
  //  z[xstart] = carry;

  Label L_first_loop, L_first_loop_exit;
  Label L_one_x, L_one_y, L_multiply;

  addic__PPC(xstart, xstart, -1);
  blt_PPC(CCR0, L_one_x);   // Special case: length of x is 1.

  // Load next two integers of x.
  sldi_PPC(tmp, xstart, LogBytesPerInt);
  ldx_PPC(x_xstart, x, tmp);
#ifdef VM_LITTLE_ENDIAN
  rldicl_PPC(x_xstart, x_xstart, 32, 0);
#endif

  align(32, 16);
  bind(L_first_loop);

  cmpdi_PPC(CCR0, idx, 1);
  blt_PPC(CCR0, L_first_loop_exit);
  addi_PPC(idx, idx, -2);
  beq_PPC(CCR0, L_one_y);

  // Load next two integers of y.
  sldi_PPC(tmp, idx, LogBytesPerInt);
  ldx_PPC(y_idx, y, tmp);
#ifdef VM_LITTLE_ENDIAN
  rldicl_PPC(y_idx, y_idx, 32, 0);
#endif


  bind(L_multiply);
  multiply64(product_high, product, x_xstart, y_idx);

  li_PPC(tmp, 0);
  addc_PPC(product, product, carry);         // Add carry to result.
  adde_PPC(product_high, product_high, tmp); // Add carry of the last addition.
  addi_PPC(kdx, kdx, -2);

  // Store result.
#ifdef VM_LITTLE_ENDIAN
  rldicl_PPC(product, product, 32, 0);
#endif
  sldi_PPC(tmp, kdx, LogBytesPerInt);
  stdx_PPC(product, z, tmp);
  mv_if_needed(carry, product_high);
  b_PPC(L_first_loop);


  bind(L_one_y); // Load one 32 bit portion of y as (0,value).

  lwz_PPC(y_idx, 0, y);
  b_PPC(L_multiply);


  bind(L_one_x); // Load one 32 bit portion of x as (0,value).

  lwz_PPC(x_xstart, 0, x);
  b_PPC(L_first_loop);

  bind(L_first_loop_exit);
}

// Multiply 64 bit by 64 bit and add 128 bit.
void MacroAssembler::multiply_add_128_x_128(Register x_xstart, Register y,
                                            Register z, Register yz_idx,
                                            Register idx, Register carry,
                                            Register product_high, Register product,
                                            Register tmp, int offset) {

  //  huge_128 product = (y[idx] * x_xstart) + z[kdx] + carry;
  //  z[kdx] = (jlong)product;

  sldi_PPC(tmp, idx, LogBytesPerInt);
  if (offset) {
    addi_PPC(tmp, tmp, offset);
  }
  ldx_PPC(yz_idx, y, tmp);
#ifdef VM_LITTLE_ENDIAN
  rldicl_PPC(yz_idx, yz_idx, 32, 0);
#endif

  multiply64(product_high, product, x_xstart, yz_idx);
  ldx_PPC(yz_idx, z, tmp);
#ifdef VM_LITTLE_ENDIAN
  rldicl_PPC(yz_idx, yz_idx, 32, 0);
#endif

  add2_with_carry(product_high, product, carry, yz_idx);

  sldi_PPC(tmp, idx, LogBytesPerInt);
  if (offset) {
    addi_PPC(tmp, tmp, offset);
  }
#ifdef VM_LITTLE_ENDIAN
  rldicl_PPC(product, product, 32, 0);
#endif
  stdx_PPC(product, z, tmp);
}

// Multiply 128 bit by 128 bit. Unrolled inner loop.
void MacroAssembler::multiply_128_x_128_loop(Register x_xstart,
                                             Register y, Register z,
                                             Register yz_idx, Register idx, Register carry,
                                             Register product_high, Register product,
                                             Register carry2, Register tmp) {

  //  jlong carry, x[], y[], z[];
  //  int kdx = ystart+1;
  //  for (int idx=ystart-2; idx >= 0; idx -= 2) { // Third loop
  //    huge_128 product = (y[idx+1] * x_xstart) + z[kdx+idx+1] + carry;
  //    z[kdx+idx+1] = (jlong)product;
  //    jlong carry2 = (jlong)(product >>> 64);
  //    product = (y[idx] * x_xstart) + z[kdx+idx] + carry2;
  //    z[kdx+idx] = (jlong)product;
  //    carry = (jlong)(product >>> 64);
  //  }
  //  idx += 2;
  //  if (idx > 0) {
  //    product = (y[idx] * x_xstart) + z[kdx+idx] + carry;
  //    z[kdx+idx] = (jlong)product;
  //    carry = (jlong)(product >>> 64);
  //  }

  Label L_third_loop, L_third_loop_exit, L_post_third_loop_done;
  const Register jdx = R0;

  // Scale the index.
  srdi__PPC(jdx, idx, 2);
  beq_PPC(CCR0, L_third_loop_exit);
  mtctr_PPC(jdx);

  align(32, 16);
  bind(L_third_loop);

  addi_PPC(idx, idx, -4);

  multiply_add_128_x_128(x_xstart, y, z, yz_idx, idx, carry, product_high, product, tmp, 8);
  mv_if_needed(carry2, product_high);

  multiply_add_128_x_128(x_xstart, y, z, yz_idx, idx, carry2, product_high, product, tmp, 0);
  mv_if_needed(carry, product_high);
  bdnz_PPC(L_third_loop);

  bind(L_third_loop_exit);  // Handle any left-over operand parts.

  andi__PPC(idx, idx, 0x3);
  beq_PPC(CCR0, L_post_third_loop_done);

  Label L_check_1;

  addic__PPC(idx, idx, -2);
  blt_PPC(CCR0, L_check_1);

  multiply_add_128_x_128(x_xstart, y, z, yz_idx, idx, carry, product_high, product, tmp, 0);
  mv_if_needed(carry, product_high);

  bind(L_check_1);

  addi_PPC(idx, idx, 0x2);
  andi__PPC(idx, idx, 0x1);
  addic__PPC(idx, idx, -1);
  blt_PPC(CCR0, L_post_third_loop_done);

  sldi_PPC(tmp, idx, LogBytesPerInt);
  lwzx_PPC(yz_idx, y, tmp);
  multiply64(product_high, product, x_xstart, yz_idx);
  lwzx_PPC(yz_idx, z, tmp);

  add2_with_carry(product_high, product, yz_idx, carry);

  sldi_PPC(tmp, idx, LogBytesPerInt);
  stwx_PPC(product, z, tmp);
  srdi_PPC(product, product, 32);

  sldi_PPC(product_high, product_high, 32);
  orr_PPC(product, product, product_high);
  mv_if_needed(carry, product);

  bind(L_post_third_loop_done);
}   // multiply_128_x_128_loop

void MacroAssembler::muladd(Register out, Register in,
                            Register offset, Register len, Register k,
                            Register tmp1, Register tmp2, Register carry) {

  // Labels
  Label LOOP, SKIP;

  // Make sure length is positive.
  cmpdi_PPC  (CCR0,    len,     0);

  // Prepare variables
  subi_PPC   (offset,  offset,  4);
  li_PPC     (carry,   0);
  ble_PPC    (CCR0,    SKIP);

  mtctr_PPC  (len);
  subi_PPC   (len,     len,     1    );
  sldi_PPC   (len,     len,     2    );

  // Main loop
  bind(LOOP);
  lwzx_PPC   (tmp1,    len,     in   );
  lwzx_PPC   (tmp2,    offset,  out  );
  mulld_PPC  (tmp1,    tmp1,    k    );
  add_PPC    (tmp2,    carry,   tmp2 );
  add_PPC    (tmp2,    tmp1,    tmp2 );
  stwx_PPC   (tmp2,    offset,  out  );
  srdi_PPC   (carry,   tmp2,    32   );
  subi_PPC   (offset,  offset,  4    );
  subi_PPC   (len,     len,     4    );
  bdnz_PPC   (LOOP);
  bind(SKIP);
}

void MacroAssembler::multiply_to_len(Register x, Register xlen,
                                     Register y, Register ylen,
                                     Register z, Register zlen,
                                     Register tmp1, Register tmp2,
                                     Register tmp3, Register tmp4,
                                     Register tmp5, Register tmp6,
                                     Register tmp7, Register tmp8,
                                     Register tmp9, Register tmp10,
                                     Register tmp11, Register tmp12,
                                     Register tmp13) {

  ShortBranchVerifier sbv(this);

  assert_different_registers(x, xlen, y, ylen, z, zlen,
                             tmp1, tmp2, tmp3, tmp4, tmp5, tmp6);
  assert_different_registers(x, xlen, y, ylen, z, zlen,
                             tmp1, tmp2, tmp3, tmp4, tmp5, tmp7);
  assert_different_registers(x, xlen, y, ylen, z, zlen,
                             tmp1, tmp2, tmp3, tmp4, tmp5, tmp8);

  const Register idx = tmp1;
  const Register kdx = tmp2;
  const Register xstart = tmp3;

  const Register y_idx = tmp4;
  const Register carry = tmp5;
  const Register product = tmp6;
  const Register product_high = tmp7;
  const Register x_xstart = tmp8;
  const Register tmp = tmp9;

  // First Loop.
  //
  //  final static long LONG_MASK = 0xffffffffL;
  //  int xstart = xlen - 1;
  //  int ystart = ylen - 1;
  //  long carry = 0;
  //  for (int idx=ystart, kdx=ystart+1+xstart; idx >= 0; idx-, kdx--) {
  //    long product = (y[idx] & LONG_MASK) * (x[xstart] & LONG_MASK) + carry;
  //    z[kdx] = (int)product;
  //    carry = product >>> 32;
  //  }
  //  z[xstart] = (int)carry;

  mv_if_needed(idx, ylen);        // idx = ylen
  mv_if_needed(kdx, zlen);        // kdx = xlen + ylen
  li_PPC(carry, 0);                   // carry = 0

  Label L_done;

  addic__PPC(xstart, xlen, -1);
  blt_PPC(CCR0, L_done);

  multiply_64_x_64_loop(x, xstart, x_xstart, y, y_idx, z,
                        carry, product_high, product, idx, kdx, tmp);

  Label L_second_loop;

  cmpdi_PPC(CCR0, kdx, 0);
  beq_PPC(CCR0, L_second_loop);

  Label L_carry;

  addic__PPC(kdx, kdx, -1);
  beq_PPC(CCR0, L_carry);

  // Store lower 32 bits of carry.
  sldi_PPC(tmp, kdx, LogBytesPerInt);
  stwx_PPC(carry, z, tmp);
  srdi_PPC(carry, carry, 32);
  addi_PPC(kdx, kdx, -1);


  bind(L_carry);

  // Store upper 32 bits of carry.
  sldi_PPC(tmp, kdx, LogBytesPerInt);
  stwx_PPC(carry, z, tmp);

  // Second and third (nested) loops.
  //
  //  for (int i = xstart-1; i >= 0; i--) { // Second loop
  //    carry = 0;
  //    for (int jdx=ystart, k=ystart+1+i; jdx >= 0; jdx--, k--) { // Third loop
  //      long product = (y[jdx] & LONG_MASK) * (x[i] & LONG_MASK) +
  //                     (z[k] & LONG_MASK) + carry;
  //      z[k] = (int)product;
  //      carry = product >>> 32;
  //    }
  //    z[i] = (int)carry;
  //  }
  //
  //  i = xlen, j = tmp1, k = tmp2, carry = tmp5, x[i] = rdx

  bind(L_second_loop);

  li_PPC(carry, 0);                   // carry = 0;

  addic__PPC(xstart, xstart, -1);     // i = xstart-1;
  blt_PPC(CCR0, L_done);

  Register zsave = tmp10;

  mr_PPC(zsave, z);


  Label L_last_x;

  sldi_PPC(tmp, xstart, LogBytesPerInt);
  add_PPC(z, z, tmp);                 // z = z + k - j
  addi_PPC(z, z, 4);
  addic__PPC(xstart, xstart, -1);     // i = xstart-1;
  blt_PPC(CCR0, L_last_x);

  sldi_PPC(tmp, xstart, LogBytesPerInt);
  ldx_PPC(x_xstart, x, tmp);
#ifdef VM_LITTLE_ENDIAN
  rldicl_PPC(x_xstart, x_xstart, 32, 0);
#endif


  Label L_third_loop_prologue;

  bind(L_third_loop_prologue);

  Register xsave = tmp11;
  Register xlensave = tmp12;
  Register ylensave = tmp13;

  mr_PPC(xsave, x);
  mr_PPC(xlensave, xstart);
  mr_PPC(ylensave, ylen);


  multiply_128_x_128_loop(x_xstart, y, z, y_idx, ylen,
                          carry, product_high, product, x, tmp);

  mr_PPC(z, zsave);
  mr_PPC(x, xsave);
  mr_PPC(xlen, xlensave);   // This is the decrement of the loop counter!
  mr_PPC(ylen, ylensave);

  addi_PPC(tmp3, xlen, 1);
  sldi_PPC(tmp, tmp3, LogBytesPerInt);
  stwx_PPC(carry, z, tmp);
  addic__PPC(tmp3, tmp3, -1);
  blt_PPC(CCR0, L_done);

  srdi_PPC(carry, carry, 32);
  sldi_PPC(tmp, tmp3, LogBytesPerInt);
  stwx_PPC(carry, z, tmp);
  b_PPC(L_second_loop);

  // Next infrequent code is moved outside loops.
  bind(L_last_x);

  lwz_PPC(x_xstart, 0, x);
  b_PPC(L_third_loop_prologue);

  bind(L_done);
}   // multiply_to_len

void MacroAssembler::asm_assert_eq(Register r1, Register r2, const char* msg, int id) {
#ifdef ASSERT
  Label ok;
  beq(r1, r2, ok);
  stop(msg, id);
  bind(ok);
#endif
}

void MacroAssembler::asm_assert_ne(Register r1, Register r2, const char* msg, int id) {
#ifdef ASSERT
  Label ok;
  bne(r1, r2, ok);
  stop(msg, id);
  bind(ok);
#endif
}

void MacroAssembler::asm_assert_mems_zero(bool check_equal, int size, int mem_offset,
                                          Register mem_base, const char* msg, int id, Register tmp) {
#ifdef ASSERT
  switch (size) {
    case 4:
      lwu(tmp, mem_base, mem_offset);
      break;
    case 8:
      ld(tmp, mem_base, mem_offset);
      break;
    default:
      ShouldNotReachHere();
  }
  Label ok;
  if (check_equal) {
    beqz(tmp, ok);
  } else {
    bnez(tmp, ok);
  }
  stop(msg, id);
  bind(ok);
#endif // ASSERT
}

void MacroAssembler::verify_thread() {
  if (VerifyThread) {
    unimplemented("'VerifyThread' currently not implemented on RISCV");
  }
}

// READ: oop. KILL: R0. Volatile floats perhaps.
void MacroAssembler::verify_oop(Register oop, const char* msg) {
#if 0  // FIXME_RISCV

  if (!VerifyOops) {
    return;
  }

  address/* FunctionDescriptor** */fd = StubRoutines::verify_oop_subroutine_entry_address();
  const Register tmp = R5_TMP0; // Will be preserved.
  const int nbytes_save = MacroAssembler::num_volatile_regs * 8;
  save_volatile_gprs(R2_SP, -nbytes_save);

  mv_if_needed(R12_ARG2, oop);
  push_frame_reg_args(nbytes_save, tmp);
  // load FunctionDescriptor** / entry_address *
  load_const_optimized(tmp, fd, R0);
  // load FunctionDescriptor* / entry_address
  ld_PPC(tmp, 0, tmp);
  load_const_optimized(R3_ARG1_PPC, (address)msg, R0);
  // Call destination for its side effect.
  call_c(tmp);

  pop_C_frame();
  restore_volatile_gprs(R1_SP_PPC, -nbytes_save); // except R0
# endif
}

void MacroAssembler::verify_oop_addr(RegisterOrConstant offs, Register base, const char* msg) {
  if (!VerifyOops) {
    return;
  }

  address/* FunctionDescriptor** */fd = StubRoutines::verify_oop_subroutine_entry_address();
  const Register tmp = R11; // Will be preserved.
  const int nbytes_save = MacroAssembler::num_volatile_regs * 8;
  save_volatile_gprs(R1_SP_PPC, -nbytes_save); // except R0

  ld_PPC(R4_ARG2_PPC, offs, base);
  save_LR_CR(tmp); // save in old frame
  push_frame_reg_args(nbytes_save, tmp);
  // load FunctionDescriptor** / entry_address *
  load_const_optimized(tmp, fd, R0);
  // load FunctionDescriptor* / entry_address
  ld_PPC(tmp, 0, tmp);
  load_const_optimized(R3_ARG1_PPC, (address)msg, R0);
  // Call destination for its side effect.
  call_c(tmp);

  pop_C_frame();
  restore_volatile_gprs(R1_SP_PPC, -nbytes_save); // except R0
}

const char* stop_types[] = {
  "stop",
  "untested",
  "unimplemented",
  "shouldnotreachhere"
};

static void stop_on_request(int tp, const char* msg) {
  tty->print("RISCV assembly code requires stop: (%s) %s\n", stop_types[tp%/*stop_end*/4], msg);
  guarantee(false, "RISCV assembly code requires stop: %s", msg);
}

// Call a C-function that prints output.
void MacroAssembler::stop(int type, const char* msg, int id) {
#ifndef PRODUCT
  block_comment(err_msg("stop: %s %s {", stop_types[type%stop_end], msg));
#else
  block_comment("stop {");
#endif

  // setup arguments
  li(R10_ARG0, type);
  li(R11_ARG1, (void *)msg);
  call_VM_leaf(CAST_FROM_FN_PTR(address, stop_on_request), R10_ARG0, R11_ARG1);
  illtrap();
  emit_int32(id);
  block_comment("} stop;");
}

#ifndef PRODUCT
// Write pattern 0x0101010101010101 in memory region [low-before, high+after].
// Val, addr are temp registers.
// If low == addr, addr is killed.
// High is preserved.
void MacroAssembler::zap_from_to(Register low, int before, Register high, int after, Register val, Register addr) {
  if (!ZapMemory) return;

  assert_different_registers(low, val);

  BLOCK_COMMENT("zap memory region {");
  load_const_optimized(val, 0x0101010101010101);
  int size = before + after;
  if (low == high && size < 5 && size > 0) {
    int offset = -before*BytesPerWord;
    for (int i = 0; i < size; ++i) {
      std_PPC(val, offset, low);
      offset += (1*BytesPerWord);
    }
  } else {
    addi_PPC(addr, low, -before*BytesPerWord);
    assert_different_registers(high, val);
    if (after) addi_PPC(high, high, after * BytesPerWord);
    Label loop;
    bind(loop);
    std_PPC(val, 0, addr);
    addi_PPC(addr, addr, 8);
    cmpd_PPC(CCR6, addr, high);
    ble_PPC(CCR6, loop);
    if (after) addi_PPC(high, high, -after * BytesPerWord);  // Correct back to old value.
  }
  BLOCK_COMMENT("} zap memory region");
}

#endif // !PRODUCT

void SkipIfEqualZero::skip_to_label_if_equal_zero(MacroAssembler* masm, Register temp,
                                                  const bool* flag_addr, Label& label) {
  int simm16_offset = masm->load_const_optimized(temp, (address)flag_addr, R0, true);
  assert(sizeof(bool) == 1, "PowerPC ABI");
  masm->lbz_PPC(temp, simm16_offset, temp);
  masm->cmpwi_PPC(CCR0, temp, 0);
  masm->beq_PPC(CCR0, label);
}

SkipIfEqualZero::SkipIfEqualZero(MacroAssembler* masm, Register temp, const bool* flag_addr) : _masm(masm), _label() {
  skip_to_label_if_equal_zero(masm, temp, flag_addr, _label);
}

SkipIfEqualZero::~SkipIfEqualZero() {
  _masm->bind(_label);
}
