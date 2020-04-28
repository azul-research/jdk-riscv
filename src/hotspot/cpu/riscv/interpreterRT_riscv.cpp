/*
 * Copyright (c) 1997, 2019, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, 2013 SAP SE. All rights reserved.
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
#include "asm/assembler.inline.hpp"
#include "interpreter/interp_masm.hpp"
#include "interpreter/interpreter.hpp"
#include "interpreter/interpreterRuntime.hpp"
#include "memory/allocation.inline.hpp"
#include "oops/method.hpp"
#include "oops/oop.inline.hpp"
#include "runtime/handles.inline.hpp"
#include "runtime/icache.hpp"
#include "runtime/interfaceSupport.inline.hpp"
#include "runtime/signature.hpp"

#define __ _masm->

// Access macros for Java and C arguments.
// The first Java argument is at index -1.
#define locals_j_arg_at(index)    R26_locals, (Interpreter::local_offset_in_bytes(index))
// The first C argument is at index 0.
#define sp_c_arg_at(index)        R2_SP, ((index) * wordSize)

// Implementation of SignatureHandlerGenerator

InterpreterRuntime::SignatureHandlerGenerator::SignatureHandlerGenerator(
    const methodHandle& method, CodeBuffer* buffer) : NativeSignatureIterator(method) {
  _masm = new MacroAssembler(buffer);
  assert(method->is_native(), "Signature handler generator only for native methods");
  _int_passed_args = method->is_static() ? 2 : 1;
  _float_passed_args = 0;
  _used_stack_words = 0;
}

void InterpreterRuntime::SignatureHandlerGenerator::pass_int() {
  Argument arg(_int_passed_args++);
  Register r = arg.is_register() ? arg.as_register() : R5_scratch1;

  __ lw(r, locals_j_arg_at(offset())); // sign extension of integer
  if (!arg.is_register()) {
    __ sd(r, sp_c_arg_at(_used_stack_words++));
  }
}

void InterpreterRuntime::SignatureHandlerGenerator::pass_long() {
  Argument arg(_int_passed_args++);
  Register r = arg.is_register() ? arg.as_register() : R5_scratch1;

  __ ld(r, locals_j_arg_at(offset() + 1)); // long resides in upper slot
  if (!arg.is_register()) {
    __ sd(r, sp_c_arg_at(_used_stack_words++));
#ifndef _LP64
  ++used_stack_words;
#endif
  }
}

void InterpreterRuntime::SignatureHandlerGenerator::pass_float() {
  FloatArgument arg(_float_passed_args++);
  FloatRegister r = arg.is_register() ? arg.as_register() : F0_TMP0;

  __ flw(r, locals_j_arg_at(offset()));
  if (jni_offset() > 8) {
    __ fsw(r, sp_c_arg_at(_used_stack_words++));
  }
}

void InterpreterRuntime::SignatureHandlerGenerator::pass_double() {
  FloatArgument arg(_float_passed_args++);
  FloatRegister r = arg.is_register() ? arg.as_register() : F0_TMP0;

  __ fld(r, locals_j_arg_at(offset() + 1));
  if (jni_offset() > 8) {
    __ fsd(r, sp_c_arg_at(_used_stack_words++));
#ifndef _LP64
    ++used_stack_words;
#endif
  }
}

void InterpreterRuntime::SignatureHandlerGenerator::pass_object() {
  Argument jni_arg(_int_passed_args++);
  Register r = jni_arg.is_register() ? jni_arg.as_register() : R5_scratch1;

  // The handle for a receiver will never be null.
  bool do_NULL_check = offset() != 0 || is_static();

  Label do_null;
  if (do_NULL_check) {
    __ ld(R6_scratch2, locals_j_arg_at(offset()));
    __ li(r, 0L);
    __ beq(R6_scratch2, R0_ZERO, do_null);
  }
  __ addi(r, locals_j_arg_at(offset()));
  __ bind(do_null);
  if (!jni_arg.is_register()) {
    __ sd(r, sp_c_arg_at(_used_stack_words++));
  }
}

void InterpreterRuntime::SignatureHandlerGenerator::generate(uint64_t fingerprint) {
  // Generate code to handle arguments.
  iterate(fingerprint);

  // Return the result handler.
  __ li(R10_RET1, AbstractInterpreter::result_handler(method()->result_type()));
  __ ret();

  __ flush();
}

#undef __

// Implementation of SignatureHandlerLibrary

void SignatureHandlerLibrary::pd_set_handler(address handler) {
}


// Access function to get the signature.
JRT_ENTRY(address, InterpreterRuntime::get_signature(JavaThread* thread, Method* method))
  methodHandle m(thread, method);
  assert(m->is_native(), "sanity check");
  Symbol *s = m->signature();
  return (address) s->base();
JRT_END

JRT_ENTRY(address, InterpreterRuntime::get_result_handler(JavaThread* thread, Method* method))
  methodHandle m(thread, method);
  assert(m->is_native(), "sanity check");
  return AbstractInterpreter::result_handler(m->result_type());
JRT_END
