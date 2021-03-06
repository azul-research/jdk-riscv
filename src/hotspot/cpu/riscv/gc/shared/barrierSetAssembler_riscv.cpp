/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2018, SAP SE. All rights reserved.
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
#include "gc/shared/barrierSetAssembler.hpp"
#include "interpreter/interp_masm.hpp"
#include "oops/compressedOops.hpp"
#include "runtime/jniHandles.hpp"

#define __ masm->

void BarrierSetAssembler::store_at(MacroAssembler* masm, DecoratorSet decorators, BasicType type,
                                   Register base, RegisterOrConstant ind_or_offs, Register val,
                                   Register tmp1, Register tmp2, Register tmp3, bool needs_frame) {
  bool in_heap = (decorators & IN_HEAP) != 0;
  bool in_native = (decorators & IN_NATIVE) != 0;
  bool not_null = (decorators & IS_NOT_NULL) != 0;
  assert(in_heap || in_native, "where?");
  assert_different_registers(base, val, tmp1, tmp2, R0);

  switch (type) {
  case T_ARRAY:
  case T_OBJECT: {
    if (UseCompressedOops && in_heap) {
      Register co = tmp1;
      if (val == noreg) {
        __ addi(co, R0, 0);
      } else {
        co = not_null ? __ encode_heap_oop_not_null(tmp1, val) : __ encode_heap_oop(tmp1, val);
      }
      __ sw(co, base, ind_or_offs, tmp2);
    } else {
      if (val == noreg) {
        val = tmp1;
        __ mv(val, R0);
      }
      __ sd(val, base, ind_or_offs, tmp2);
    }
    break;
  }
  default: Unimplemented();
  }
}

void BarrierSetAssembler::load_at(MacroAssembler* masm, DecoratorSet decorators, BasicType type,
                                  Register base, RegisterOrConstant ind_or_offs, Register dst,
                                  Register tmp1, Register tmp2, bool needs_frame, Label *L_handle_null) {
  bool in_heap = (decorators & IN_HEAP) != 0;
  bool in_native = (decorators & IN_NATIVE) != 0;
  bool not_null = (decorators & IS_NOT_NULL) != 0;
  assert(in_heap || in_native, "where?");
  assert_different_registers(ind_or_offs.register_or_noreg(), dst, R0);

  switch (type) {
  case T_ARRAY:
  case T_OBJECT: {
    if (UseCompressedOops && in_heap) {
      if (L_handle_null != NULL) { // Label provided.
        __ lwu(dst, base, ind_or_offs);
        __ beqz(dst, *L_handle_null);
        __ decode_heap_oop_not_null(dst);
      } else if (not_null) { // Guaranteed to be not null.
        Register narrowOop = (tmp1 != noreg && CompressedOops::base_disjoint()) ? tmp1 : dst;
        __ lwu(narrowOop, base, ind_or_offs);
        __ decode_heap_oop_not_null(dst, narrowOop);
      } else { // Any oop.
        __ lwu(dst, base, ind_or_offs);
        __ decode_heap_oop(dst);
      }
    } else {
      __ ld(dst, base, ind_or_offs);
      if (L_handle_null != NULL) {
        __ beqz(dst, *L_handle_null);
      }
    }
    break;
  }
  default: Unimplemented();
  }
}

void BarrierSetAssembler::resolve_jobject(MacroAssembler* masm, Register value,
                                          Register tmp1, Register tmp2, bool needs_frame) {
  Label done;
  __ beqz(value, done);       // Use NULL as-is.

  __ andi(tmp1, value, -(1 << JNIHandles::weak_tag_size));
  __ ld(value, tmp1, 0);      // Resolve (untagged) jobject.

  __ verify_oop(value);
  __ bind(done);
}
