/*
 * Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2015, 2019, SAP SE. All rights reserved.
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
#include "interpreter/bytecodeHistogram.hpp"
#include "interpreter/interpreter.hpp"
#include "interpreter/interpreterRuntime.hpp"
#include "interpreter/interp_masm.hpp"
#include "interpreter/templateInterpreterGenerator.hpp"
#include "interpreter/templateTable.hpp"
#include "oops/arrayOop.hpp"
#include "oops/methodData.hpp"
#include "oops/method.hpp"
#include "oops/oop.inline.hpp"
#include "prims/jvmtiExport.hpp"
#include "prims/jvmtiThreadState.hpp"
#include "runtime/arguments.hpp"
#include "runtime/deoptimization.hpp"
#include "runtime/frame.inline.hpp"
#include "runtime/sharedRuntime.hpp"
#include "runtime/stubRoutines.hpp"
#include "runtime/synchronizer.hpp"
#include "runtime/timer.hpp"
#include "runtime/vframeArray.hpp"
#include "utilities/debug.hpp"
#include "utilities/macros.hpp"

#undef __
#define __ _masm->

// Size of interpreter code.  Increase if too small.  Interpreter will
// fail with a guarantee ("not enough space for interpreter generation");
// if too small.
// Run with +PrintInterpreter to get the VM to print out the size.
// Max size with JVMTI
int TemplateInterpreter::InterpreterCodeSize = 256*K;

#ifdef PRODUCT
#define BLOCK_COMMENT(str) /* nothing */
#else
#define BLOCK_COMMENT(str) __ block_comment(str)
#endif

#define BIND(label)        __ bind(label); BLOCK_COMMENT(#label ":")

//-----------------------------------------------------------------------------

address TemplateInterpreterGenerator::generate_slow_signature_handler() {
  // Slow_signature handler that respects the RISCV C calling conventions.
  //
  // We get called by the native entry code with our output register
  // area == 8. First we call InterpreterRuntime::get_result_handler
  // to copy the pointer to the signature string temporarily to the
  // first C-argument and to return the result_handler in
  // R10_RET. Since native_entry will copy the jni-pointer to the
  // first C-argument slot later on, it is OK to occupy this slot
  // temporarilly. Then we copy the argument list on the java
  // expression stack into native varargs format on the native stack
  // and load arguments into argument registers. Integer arguments in
  // the varargs vector will be sign-extended to 8 bytes.
  //
  // On entry:
  //   R10_ARG0       - intptr_t*     Address of java argument list in memory.
  //   R15_prev_state - BytecodeInterpreter* Address of interpreter state for this method
  //   R27_method
  //
  // On exit (just before return instruction):
  //   R10_RET            - contains the address of the result_handler.
  //   R11_ARG1           - is not updated for static methods and contains "this" otherwise.
  //   R12_ARG2-R17_ARG7: - When the (i-2)th Java argument is not of type float or double,
  //                        ARGi contains this argument. Otherwise, ARGi is not updated.
  //   F10_ARG0-F17_ARG7: - contain the first 8 arguments of type float or double.

  const int LogSizeOfTwoInstructions = 3; // TODO_RISCV check

  const int max_fp_register_arguments  = FloatArgument::n_float_register_parameters;
  const int max_int_register_arguments = Argument::n_int_register_parameters - 2;  // first 2 are reserved

#ifdef ASSERT
  const Register type_mask      = R5_scratch1;
  const Register abs_arg        = R6_scratch2;
#endif
  const Register arg_java       = R23_esp; // we should restore this register in the end
  const Register arg_c          = R7_TMP2;
  const Register signature      = R22_bcp; // we should restore this register in the end
  const Register sig_byte       = R28_TMP3;
  const Register fpcnt          = R29_TMP4;
  const Register ipcnt          = R30_TMP5;
  const Register intSlot        = R31_TMP6;
  const FloatRegister floatSlot = F0_TMP0;

  assert(arg_java->is_nonvolatile(), "arg_java should be nonvolatile");
  assert(signature->is_nonvolatile(), "signature should be nonvolatile");

  address entry = __ pc();

  __ sd(R1_RA, R8_FP, _ijava_state(saved_ra));

  __ mv(arg_java, R10_ARG0);

  __ call_VM_leaf(CAST_FROM_FN_PTR(address, InterpreterRuntime::get_signature), R24_thread, R27_method);

  // Signature is in R10_RET1. Signature is callee saved.
  __ mv(signature, R10_RET1);

  // Get the result handler.
  __ call_VM_leaf(CAST_FROM_FN_PTR(address, InterpreterRuntime::get_result_handler), R24_thread, R27_method);

  {
    Label Lstatic_method;
    // test if static
    // _access_flags._flags must be at offset 0.
    // TODO RISCV port: requires change in shared code.
    //assert(in_bytes(AccessFlags::flags_offset()) == 0,
    //       "MethodDesc._access_flags == MethodDesc._access_flags._flags");
    // _access_flags must be a 32 bit value.
    assert(sizeof(AccessFlags) == 4, "wrong size");
    __ lwu(R5_scratch1/*access_flags*/, method_(access_flags));
    // testbit with condition register.
    __ andi(R5_scratch1, R5_scratch1, 1 << JVM_ACC_STATIC_BIT);
    __ bnez(R5_scratch1, Lstatic_method);
    // For non-static functions, pass "this" in R11_ARG1
    // We need to box the Java object here, so we use arg_java
    // (address of current Java stack slot) as argument and don't
    // dereference it as in case of ints, floats, etc.
    __ mv(R11_ARG1, arg_java);
    __ addi(arg_java, arg_java, -BytesPerWord);
    __ bind(Lstatic_method);
  }

  // two arguments are reserved, but we don't count it here
  __ li(ipcnt, 0L);
  // arg_c points to first C argument on the stack
  __ mv(arg_c, R2_SP);
  // no floating-point args parsed so far
  __ li(fpcnt, 0L);

  Label move_intSlot_to_ARG, move_floatSlot_to_FARG;
  Label loop_start, loop_end;
  Label do_int, do_long, do_float, do_double, do_dontreachhere, do_object, do_array, do_boxed;

  // signature points to '(' at entry
#ifdef ASSERT
  __ lbu(sig_byte, signature, 0);
  __ li(R5_scratch1, (long) '(');
  __ bne(sig_byte, R5_scratch1, do_dontreachhere);
#endif

  __ bind(loop_start); // TODO_RISCV there is still PPC code. I should rewrite it with RISC-V calling conventions

  __ lbu(sig_byte, signature, 1);
  __ addi(signature, signature, 1);

  __ li(R5_scratch1, (long) ')'); // end of signature
  __ beq(sig_byte, R5_scratch1, loop_end);

  __ li(R5_scratch1, (long) 'B'); // byte
  __ beq(sig_byte, R5_scratch1, do_int);

  __ li(R5_scratch1, (long) 'C'); // char
  __ beq(sig_byte, R5_scratch1, do_int);

  __ li(R5_scratch1, (long) 'D'); // double
  __ beq(sig_byte, R5_scratch1, do_double);

  __ li(R5_scratch1, (long) 'F'); // float
  __ beq(sig_byte, R5_scratch1, do_float);

  __ li(R5_scratch1, (long) 'I'); // int
  __ beq(sig_byte, R5_scratch1, do_int);

  __ li(R5_scratch1, (long) 'J'); // long
  __ beq(sig_byte, R5_scratch1, do_long);

  __ li(R5_scratch1, (long) 'S'); // short
  __ beq(sig_byte, R5_scratch1, do_int);

  __ li(R5_scratch1, (long) 'Z'); // boolean
  __ beq(sig_byte, R5_scratch1, do_int);

  __ li(R5_scratch1, (long) 'L'); // object
  __ beq(sig_byte, R5_scratch1, do_object);

  __ li(R5_scratch1, (long) '['); // array
  __ beq(sig_byte, R5_scratch1, do_array);

  //  __ li(R5_scratch1, 'V'); // void cannot appear since we do not parse the return type
  //  __ beq(sig_byte, R5_scratch1, do_void);

  __ bind(do_dontreachhere);

  __ unimplemented("ShouldNotReachHere in slow_signature_handler", 120);

  __ bind(do_array);
  {
    Label start_skip, end_skip;

    __ bind(start_skip);
    __ lbu(sig_byte, signature, 1);
    __ addi(signature, signature, 1);

    __ li(R5_scratch1, (long) '[');
    __ beq(sig_byte, R5_scratch1, start_skip); // skip further brackets

    __ li(R5_scratch1, (long) '9');
    __ bgt(sig_byte, R5_scratch1, end_skip);   // no optional size

    __ li(R5_scratch1, (long) '0');
    __ bge(sig_byte, R5_scratch1, start_skip); // skip optional size
    __ bind(end_skip);

    __ li(R5_scratch1, (long) 'L');
    __ beq(sig_byte, R5_scratch1, do_object);  // for arrays of objects, the name of the object must be skipped
    __ j(do_boxed);          // otherwise, go directly to do_boxed
  }

  __ bind(do_object);
  {
    Label L;
    __ bind(L);
    __ lbu(sig_byte, signature, 1);
    __ addi(signature, signature, 1);
    __ li(R5_scratch1, (long) ';');
    __ bne(sig_byte, R5_scratch1, L);
   }
  // Need to box the Java object here, so we use arg_java (address of
  // current Java stack slot) as argument and don't dereference it as
  // in case of ints, floats, etc.
  Label do_null;
  __ bind(do_boxed);
  {
    __ ld(R5_scratch1, arg_java, 0);
    __ li(intSlot, 0L);
    __ beqz(R5_scratch1, do_null);
    __ mv(intSlot, arg_java);

    __ bind(do_null);
    __ addi(arg_java, arg_java, -BytesPerWord);
    __ li(R5_scratch1, max_int_register_arguments);
    __ blt(ipcnt, R5_scratch1, move_intSlot_to_ARG);
    __ sd(intSlot, arg_c, 0);
    __ addi(arg_c, arg_c, BytesPerWord);
    __ j(loop_start);
  }

  __ bind(do_int);
  {
    __ lw(intSlot, arg_java, 0);
    __ addi(arg_java, arg_java, -BytesPerWord);
    __ li(R5_scratch1, max_int_register_arguments);
    __ blt(ipcnt, R5_scratch1, move_intSlot_to_ARG);
    __ sd(intSlot, arg_c, 0);
    __ addi(arg_c, arg_c, BytesPerWord);
    __ j(loop_start);
  }

  __ bind(do_long);
  {
    __ ld(intSlot, arg_java, -BytesPerWord);
    __ addi(arg_java, arg_java, -2 * BytesPerWord);
    __ li(R5_scratch1, max_int_register_arguments);
    __ blt(ipcnt, R5_scratch1, move_intSlot_to_ARG);
    __ sd(intSlot, arg_c, 0);
    __ addi(arg_c, arg_c, BytesPerWord);
    __ j(loop_start);
  }

  __ bind(do_float);
  {
    __ flw(floatSlot, arg_java, 0);
    __ addi(arg_java, arg_java, -BytesPerWord);
    __ li(R5_scratch1, max_fp_register_arguments);
    __ blt(ipcnt, R5_scratch1, move_floatSlot_to_FARG);
#ifdef VM_LITTLE_ENDIAN
    __ fsw(floatSlot, arg_c, 0);
#elif
#warning "unchecked BIGENDIAN code in TemplateInterpreterGenerator::generate_slow_signature_handler"
    __ fsw(floatSlot, arg_c, 4); // TODO RISCV check this
#endif
    __ addi(arg_c, arg_c, BytesPerWord);
    __ j(loop_start);
  }

  __ bind(do_double);
  {
    __ fld(floatSlot, arg_java, -BytesPerWord);
    __ addi(arg_java, arg_java, -2 * BytesPerWord);
    __ li(R5_scratch1, max_fp_register_arguments);
    __ blt(ipcnt, R5_scratch1, move_floatSlot_to_FARG);
    __ fsd(floatSlot, arg_c, 0);
    __ addi(arg_c, arg_c, BytesPerWord);
    __ j(loop_start);
  }

  __ bind(loop_end);

  assert(signature == R22_bcp, "We should restore bcp");
  assert(arg_java == R23_esp, "We should restore esp");
  __ ld(R22_bcp, R8_FP, _ijava_state(bcp));
  __ ld(R23_esp, R8_FP, _ijava_state(esp));
  __ ld(R1_RA, R8_FP, _ijava_state(saved_ra));
  __ ret();

  Label Lmove_int_arg, Lmove_float_arg;
  __ bind(Lmove_int_arg); // each case must consist of 2 instructions (otherwise adapt LogSizeOfTwoInstructions)
  __ mv(R12_ARG2, intSlot); __ j(loop_start);
  __ mv(R13_ARG3, intSlot); __ j(loop_start);
  __ mv(R14_ARG4, intSlot); __ j(loop_start);
  __ mv(R15_ARG5, intSlot); __ j(loop_start);
  __ mv(R16_ARG6, intSlot); __ j(loop_start);
  __ mv(R17_ARG7, intSlot); __ j(loop_start);

  __ bind(Lmove_float_arg); // each case must consist of 2 instructions (otherwise adapt LogSizeOfTwoInstructions)
  __ fmvd(F10_ARG0, floatSlot); __ j(loop_start);
  __ fmvd(F11_ARG1, floatSlot); __ j(loop_start);
  __ fmvd(F12_ARG2, floatSlot); __ j(loop_start);
  __ fmvd(F13_ARG3, floatSlot); __ j(loop_start);
  __ fmvd(F14_ARG4, floatSlot); __ j(loop_start);
  __ fmvd(F15_ARG5, floatSlot); __ j(loop_start);
  __ fmvd(F16_ARG6, floatSlot); __ j(loop_start);
  __ fmvd(F17_ARG7, floatSlot); __ j(loop_start);

  __ bind(move_intSlot_to_ARG);
  __ slli(R5_scratch1, ipcnt, LogSizeOfTwoInstructions);
  __ addi(ipcnt, ipcnt, 1);
  __ add_const_optimized(R6_scratch2, R5_scratch1, Lmove_int_arg);  // Label must be bound here.
  __ jr(R6_scratch2/*branch_target*/);

  __ bind(move_floatSlot_to_FARG);
  __ slli(R5_scratch1, fpcnt, LogSizeOfTwoInstructions);
  __ addi(fpcnt, fpcnt, 1);
  __ add_const_optimized(R6_scratch2, R5_scratch1, Lmove_float_arg);  // Label must be bound here.
  __ jr(R6_scratch2/*branch_target*/);

  return entry;
}

address TemplateInterpreterGenerator::generate_result_handler_for(BasicType type) {
  Label done;
  address entry = __ pc();

  switch (type) {
  case T_BOOLEAN:
    // convert !=0 to 1
    __ neg(R5_scratch1, R10_RET1);
    __ orr(R5_scratch1, R10_RET1, R5_scratch1); // R5_scratch1 <= 0 && (R5_scratch1 == 0 <-> R10_RET1 == 0)
    __ slti(R10_RET1, R5_scratch1, 0);          // R10_RET1 == 1 <-> R5_scratch1 < 0,    R10_RET1 == 0 otherwise
    break;
  case T_BYTE:
    __ signExtend(R10_RET1, 8);
     break;
  case T_CHAR:
     __ zeroExtend(R10_RET1, 16);
     break;
  case T_SHORT:
    __ signExtend(R10_RET1, 16);
     break;
  case T_INT:
    __ signExtend(R10_RET1, 32);
     break;
  case T_LONG:
     break;
  case T_OBJECT:
    // JNIHandles::resolve result.
    __ resolve_jobject(R10_RET1, R5_scratch1, R6_scratch2, true /* needs_frame */);
    break;
  case T_FLOAT:
     break;
  case T_DOUBLE:
     break;
  case T_VOID:
     tty->print_cr("generate_result_handler(void) %p", __ pc());
     break;
  default: ShouldNotReachHere();
  }

  BIND(done);
  __ ret();

  return entry;
}

// Abstract method entry.
//
address TemplateInterpreterGenerator::generate_abstract_entry(void) {
  address entry = __ pc();

  //
  // Registers alive
  //   R24_thread     - JavaThread*
  //   R27_method     - callee's method (method to be invoked)
  //   R1_SP_PPC          - SP prepared such that caller's outgoing args are near top
  //   LR             - return address to caller
  //
  // Stack layout at this point:
  //
  //   0       [TOP_IJAVA_FRAME_ABI]         <-- R1_SP_PPC
  //           alignment (optional)
  //           [outgoing Java arguments]
  //           ...
  //   PARENT  [PARENT_IJAVA_FRAME_ABI]
  //            ...
  //

  // Can't use call_VM here because we have not set up a new
  // interpreter state. Make the call to the vm and make it look like
  // our caller set up the JavaFrameAnchor.
  __ set_top_ijava_frame_at_SP_as_last_Java_frame(R1_SP_PPC, noreg, R6_scratch2/*tmp*/);

  // Push a new C frame and save LR.
  __ save_LR_CR(R0);
  __ push_frame_reg_args(0, R5_scratch1);

  // This is not a leaf but we have a JavaFrameAnchor now and we will
  // check (create) exceptions afterward so this is ok.
  __ call_VM_leaf(CAST_FROM_FN_PTR(address, InterpreterRuntime::throw_AbstractMethodErrorWithMethod),
                  R24_thread, R27_method);

  // Pop the C frame and restore RA.
  __ pop_C_frame();

  // Reset JavaFrameAnchor from call_VM_leaf above.
  __ reset_last_Java_frame();

  // We don't know our caller, so jump to the general forward exception stub,
  // which will also pop our full frame off. Satisfy the interface of
  // SharedRuntime::generate_forward_exception()
  __ load_const_optimized(R5_scratch1, StubRoutines::forward_exception_entry(), R0);
  __ jr(R5_scratch1);

  return entry;
}

// Interpreter intrinsic for WeakReference.get().
// 1. Don't push a full blown frame and go on dispatching, but fetch the value
//    into R8 and return quickly
// 2. If G1 is active we *must* execute this intrinsic for corrrectness:
//    It contains a GC barrier which puts the reference into the satb buffer
//    to indicate that someone holds a strong reference to the object the
//    weak ref points to!
address TemplateInterpreterGenerator::generate_Reference_get_entry(void) {
  // Code: _aload_0, _getfield, _areturn
  // parameter size = 1
  //
  // The code that gets generated by this routine is split into 2 parts:
  //    1. the "intrinsified" code for G1 (or any SATB based GC),
  //    2. the slow path - which is an expansion of the regular method entry.
  //
  // Notes:
  // * In the G1 code we do not check whether we need to block for
  //   a safepoint. If G1 is enabled then we must execute the specialized
  //   code for Reference.get (except when the Reference object is null)
  //   so that we can log the value in the referent field with an SATB
  //   update buffer.
  //   If the code for the getfield template is modified so that the
  //   G1 pre-barrier code is executed when the current method is
  //   Reference.get() then going through the normal method entry
  //   will be fine.
  // * The G1 code can, however, check the receiver object (the instance
  //   of java.lang.Reference) and jump to the slow path if null. If the
  //   Reference object is null then we obviously cannot fetch the referent
  //   and so we don't need to call the G1 pre-barrier. Thus we can use the
  //   regular method entry code to generate the NPE.
  //

  address entry = __ pc();

  const int referent_offset = java_lang_ref_Reference::referent_offset;
  guarantee(referent_offset > 0, "referent offset not initialized");

  Label slow_path;

  // Debugging not possible, so can't use __ skip_if_jvmti_mode(slow_path, GR31_SCRATCH);

  // In the G1 code we don't check if we need to reach a safepoint. We
  // continue and the thread will safepoint at the next bytecode dispatch.

  // If the receiver is null then it is OK to jump to the slow path.
  __ ld(R10_RET1, R23_esp, Interpreter::stackElementSize); // get receiver

  // Check if receiver == NULL and go the slow path.
  __ beq(R10_RET1, R0, slow_path);

  __ load_heap_oop(R10_RET1, referent_offset, R10_RET1, // TODO RISCV R7_TMP2 is volatile. Check that it is ok
                   /* non-volatile temp */ R7_TMP2, R5_scratch1, true, ON_WEAK_OOP_REF);

  // Generate the G1 pre-barrier code to log the value of
  // the referent field in an SATB buffer. Note with
  // these parameters the pre-barrier does not generate
  // the load of the previous value.

  // Restore caller sp for c2i case (from compiled) and for resized sender frame (from interpreted).
  __ resize_frame_absolute(R21_sender_SP, R5_scratch1);

  __ ret();

  __ bind(slow_path);

  __ jump_to_entry(Interpreter::entry_for_kind(Interpreter::zerolocals), R5_scratch1);


  return entry;
}

address TemplateInterpreterGenerator::generate_StackOverflowError_handler() {
  address entry = __ pc();

  // Expression stack must be empty before entering the VM if an
  // exception happened.
  __ empty_expression_stack();
  // Throw exception.
  __ call_VM(noreg,
             CAST_FROM_FN_PTR(address,
                              InterpreterRuntime::throw_StackOverflowError));
  return entry;
}

address TemplateInterpreterGenerator::generate_ArrayIndexOutOfBounds_handler() {
  address entry = __ pc();
  __ empty_expression_stack();
  // R4_ARG2_PPC already contains the array.
  // Index is in R25_tos.
  __ mr_PPC(R5_ARG3_PPC, R25_tos);
  __ call_VM(noreg, CAST_FROM_FN_PTR(address, InterpreterRuntime::throw_ArrayIndexOutOfBoundsException), R4_ARG2_PPC, R5_ARG3_PPC);
  return entry;
}

address TemplateInterpreterGenerator::generate_ClassCastException_handler() {
  address entry = __ pc();
  // Expression stack must be empty before entering the VM if an
  // exception happened.
  __ empty_expression_stack();

  // Load exception object.
  // Thread will be loaded to R3_ARG1_PPC.
  __ call_VM(noreg, CAST_FROM_FN_PTR(address, InterpreterRuntime::throw_ClassCastException), R25_tos);
#ifdef ASSERT
  // Above call must not return here since exception pending.
  __ should_not_reach_here();
#endif
  return entry;
}

address TemplateInterpreterGenerator::generate_exception_handler_common(const char* name, const char* message, bool pass_oop) {
  address entry = __ pc();
  __ untested("generate_exception_handler_common");
  Register Rexception = R25_tos;

  // Expression stack must be empty before entering the VM if an exception happened.
  __ empty_expression_stack();

  __ load_const_optimized(R12_ARG2, (address) name, R5_scratch1);
  if (pass_oop) {
    __ mr_PPC(R5_ARG3_PPC, Rexception);
    __ call_VM(Rexception, CAST_FROM_FN_PTR(address, InterpreterRuntime::create_klass_exception), false);
  } else {
    __ load_const_optimized(R13_ARG3, (address) message, R5_scratch1);
    __ call_VM(Rexception, CAST_FROM_FN_PTR(address, InterpreterRuntime::create_exception), false);
  }

  // Throw exception.
  __ mr_PPC(R3_ARG1_PPC, Rexception);
  __ load_const_optimized(R5_scratch1, Interpreter::throw_exception_entry(), R6_scratch2);
  __ mtctr_PPC(R5_scratch1);
  __ bctr_PPC();

  return entry;
}

// This entry is returned to when a call returns to the interpreter.
// When we arrive here, we expect that the callee stack frame is already popped.
address TemplateInterpreterGenerator::generate_return_entry_for(TosState state, int step, size_t index_size) {
  address entry = __ pc();

  // Move the value out of the return register back to the TOS cache of current frame.
  switch (state) {
    case ltos:
    case btos:
    case ztos:
    case ctos:
    case stos:
    case atos:
    case itos: __ mv(R25_tos, R10_RET1); break;   // RET -> TOS cache
    case ftos: __ fmvs(F23_ftos, F10_RET); break;
    case dtos: __ fmvd(F23_ftos, F10_RET); break; // TOS cache -> GR_FRET
    case vtos: break;                           // Nothing to do, this was a void return.
    default  : ShouldNotReachHere();
  }

  __ restore_interpreter_state();

    if (state == 9) {
      printf("return entry-3: %p %d %d\n", __ pc(), state, atos);
  }


  // Resize frame to top_frame_sp
  __ ld(R2_SP, R8_FP, _ijava_state(top_frame_sp));

  // Compiled code destroys templateTableBase, reload.
  __ li(R19_templateTableBase, (address)Interpreter::dispatch_table((TosState)0));

  if (state == atos) {
 //   __ unimplemented("return for atos is not implemented");
  //  __ profile_return_type(R3_RET_PPC, R5_scratch1, R6_scratch2);
  }

  const Register cache = R5_scratch1;
  const Register size  = R6_scratch2;
  __ get_cache_and_index_at_bcp(cache, 1, index_size);

  // Get least significant byte of 64 bit value:
#if defined(VM_LITTLE_ENDIAN)
  __ lbu(size, cache, in_bytes(ConstantPoolCache::base_offset() + ConstantPoolCacheEntry::flags_offset()));
#else
  __ lbu(size, cache, in_bytes(ConstantPoolCache::base_offset() + ConstantPoolCacheEntry::flags_offset()) + 7);
#endif
  __ slli(size, size, Interpreter::logStackElementSize);
  __ add(R23_esp, R23_esp, size);

  __ check_and_handle_popframe(R5_scratch1, R6_scratch2);
  __ check_and_handle_earlyret(R5_scratch1, R6_scratch2);

  __ dispatch_next(state, step);
  return entry;
}

address TemplateInterpreterGenerator::generate_deopt_entry_for(TosState state, int step, address continuation) {
  address entry = __ pc();
  // If state != vtos, we're returning from a native method, which put it's result
  // into the result register. So move the value out of the return register back
  // to the TOS cache of current frame.

  switch (state) {
    case ltos:
    case btos:
    case ztos:
    case ctos:
    case stos:
    case atos:
    case itos: __ mr_PPC(R25_tos, R3_RET_PPC); break;   // GR_RET -> TOS cache
    case ftos:
    case dtos: __ fmr_PPC(F23_ftos, F1_RET_PPC); break; // TOS cache -> GR_FRET
    case vtos: break;                           // Nothing to do, this was a void return.
    default  : ShouldNotReachHere();
  }

  // Load LcpoolCache @@@ should be already set!
  __ get_constant_pool_cache(R9_constPoolCache);

  // Handle a pending exception, fall through if none.
  __ check_and_forward_exception(R5_scratch1, R6_scratch2);

  // Start executing bytecodes.
  if (continuation == NULL) {
    __ dispatch_next(state, step);
  } else {
    __ jump_to_entry(continuation, R5_scratch1);
  }

  return entry;
}

address TemplateInterpreterGenerator::generate_safept_entry_for(TosState state, address runtime_entry) {
  address entry = __ pc();

  __ push(state);
  __ call_VM(noreg, runtime_entry);
  __ dispatch_via(vtos, Interpreter::_normal_table.table_for(vtos));

  return entry;
}

// Helpers for commoning out cases in the various type of method entries.

// Increment invocation count & check for overflow.
//
// Note: checking for negative value instead of overflow
//       so we have a 'sticky' overflow test.
//
void TemplateInterpreterGenerator::generate_counter_incr(Label* overflow, Label* profile_method, Label* profile_method_continue) {
  // Note: In tiered we increment either counters in method or in MDO depending if we're profiling or not.
  Register Rscratch1   = R5_scratch1;
  Register Rscratch2   = R6_scratch2;
  Register R3_counters = R3_ARG1_PPC;
  Label done;

  if (TieredCompilation) {
    const int increment = InvocationCounter::count_increment;
    Label no_mdo;
    if (ProfileInterpreter) {
      const Register Rmdo = R3_counters;
      // If no method data exists, go to profile_continue.
      __ ld_PPC(Rmdo, in_bytes(Method::method_data_offset()), R27_method);
      __ cmpdi_PPC(CCR0, Rmdo, 0);
      __ beq_PPC(CCR0, no_mdo);

      // Increment invocation counter in the MDO.
      const int mdo_ic_offs = in_bytes(MethodData::invocation_counter_offset()) + in_bytes(InvocationCounter::counter_offset());
      __ lwz_PPC(Rscratch2, mdo_ic_offs, Rmdo);
      __ lwz_PPC(Rscratch1, in_bytes(MethodData::invoke_mask_offset()), Rmdo);
      __ addi_PPC(Rscratch2, Rscratch2, increment);
      __ stw_PPC(Rscratch2, mdo_ic_offs, Rmdo);
      __ and__PPC(Rscratch1, Rscratch2, Rscratch1);
      __ bne_PPC(CCR0, done);
      __ b_PPC(*overflow);
    }

    // Increment counter in MethodCounters*.
    const int mo_ic_offs = in_bytes(MethodCounters::invocation_counter_offset()) + in_bytes(InvocationCounter::counter_offset());
    __ bind(no_mdo);
    __ get_method_counters(R27_method, R3_counters, done);
    __ lwz_PPC(Rscratch2, mo_ic_offs, R3_counters);
    __ lwz_PPC(Rscratch1, in_bytes(MethodCounters::invoke_mask_offset()), R3_counters);
    __ addi_PPC(Rscratch2, Rscratch2, increment);
    __ stw_PPC(Rscratch2, mo_ic_offs, R3_counters);
    __ and__PPC(Rscratch1, Rscratch2, Rscratch1);
    __ beq_PPC(CCR0, *overflow);

    __ bind(done);

  } else {

    // Update standard invocation counters.
    Register Rsum_ivc_bec = R4_ARG2_PPC;
    __ get_method_counters(R27_method, R3_counters, done);
    __ increment_invocation_counter(R3_counters, Rsum_ivc_bec, R6_scratch2);
    // Increment interpreter invocation counter.
    if (ProfileInterpreter) {  // %%% Merge this into methodDataOop.
      __ lwz_PPC(R6_scratch2, in_bytes(MethodCounters::interpreter_invocation_counter_offset()), R3_counters);
      __ addi_PPC(R6_scratch2, R6_scratch2, 1);
      __ stw_PPC(R6_scratch2, in_bytes(MethodCounters::interpreter_invocation_counter_offset()), R3_counters);
    }
    // Check if we must create a method data obj.
    if (ProfileInterpreter && profile_method != NULL) {
      const Register profile_limit = Rscratch1;
      __ lwz_PPC(profile_limit, in_bytes(MethodCounters::interpreter_profile_limit_offset()), R3_counters);
      // Test to see if we should create a method data oop.
      __ cmpw_PPC(CCR0, Rsum_ivc_bec, profile_limit);
      __ blt_PPC(CCR0, *profile_method_continue);
      // If no method data exists, go to profile_method.
      __ test_method_data_pointer(*profile_method);
    }
    // Finally check for counter overflow.
    if (overflow) {
      const Register invocation_limit = Rscratch1;
      __ lwz_PPC(invocation_limit, in_bytes(MethodCounters::interpreter_invocation_limit_offset()), R3_counters);
      __ cmpw_PPC(CCR0, Rsum_ivc_bec, invocation_limit);
      __ bge_PPC(CCR0, *overflow);
    }

    __ bind(done);
  }
}

// Generate code to initiate compilation on invocation counter overflow.
void TemplateInterpreterGenerator::generate_counter_overflow(Label& continue_entry) {
  // Generate code to initiate compilation on the counter overflow.

  // InterpreterRuntime::frequency_counter_overflow takes one arguments,
  // which indicates if the counter overflow occurs at a backwards branch (NULL bcp)
  // We pass zero in.
  // The call returns the address of the verified entry point for the method or NULL
  // if the compilation did not complete (either went background or bailed out).
  //
  // Unlike the C++ interpreter above: Check exceptions!
  // Assumption: Caller must set the flag "do_not_unlock_if_sychronized" if the monitor of a sync'ed
  // method has not yet been created. Thus, no unlocking of a non-existing monitor can occur.

  __ call_VM(noreg, CAST_FROM_FN_PTR(address, InterpreterRuntime::frequency_counter_overflow), R0, true);

  // Returns verified_entry_point or NULL.
  // We ignore it in any case.
  __ j(continue_entry);
}

// See if we've got enough room on the stack for locals plus overhead below
// JavaThread::stack_overflow_limit(). If not, throw a StackOverflowError
// without going through the signal handler, i.e., reserved and yellow zones
// will not be made usable. The shadow zone must suffice to handle the
// overflow.
//
// Kills Rscratch1.
void TemplateInterpreterGenerator::generate_stack_overflow_check(Register Rnext_SP, Register Rscratch1) {
  Label done;
  assert_different_registers(Rnext_SP, Rscratch1);

  BLOCK_COMMENT("stack_overflow_check_with_compare {");
  __ ld(Rscratch1, thread_(stack_overflow_limit));
  __ bgt(Rnext_SP, Rscratch1, done);

  // The stack overflows. Load target address of the runtime stub and call it.
  assert(StubRoutines::throw_StackOverflowError_entry() != NULL, "generated in wrong order");
  __ li(Rscratch1, (long)(unsigned long)(StubRoutines::throw_StackOverflowError_entry()));
  // Restore caller_sp (c2i adapter may exist, but no shrinking of interpreted caller frame).
#ifdef ASSERT // FIXME_RISCV
//  Label frame_not_shrunk;
//  __ cmpld_PPC(CCR0, R2_SP, R21_sender_SP);
//  __ ble_PPC(CCR0, frame_not_shrunk);
//  __ stop("frame shrunk", 0x546);
//  __ bind(frame_not_shrunk);
//  __ ld_PPC(Rscratch1, 0, R2_SP);
//  __ ld_PPC(R0, 0, R21_sender_SP);
//  __ cmpd_PPC(CCR0, R0, Rscratch1);
//  __ asm_assert_eq("backlink", 0x547);
#endif // ASSERT
  __ mv(R2_SP, R21_sender_SP);
  __ jr(Rscratch1);

  __ align(32, 12);
  __ bind(done);
  BLOCK_COMMENT("} stack_overflow_check_with_compare");
}

// Lock the current method, interpreter register window must be set up!
void TemplateInterpreterGenerator::lock_method(Register Rflags, Register Rscratch1, Register Rscratch2, bool flags_preloaded) {
  const Register Robj_to_lock = Rscratch2;

  {
    if (!flags_preloaded) {
      __ lwz_PPC(Rflags, method_PPC(access_flags));
    }

#ifdef ASSERT
    // Check if methods needs synchronization.
    {
      Label Lok;
      __ testbitdi_PPC(CCR0, R0, Rflags, JVM_ACC_SYNCHRONIZED_BIT);
      __ btrue_PPC(CCR0,Lok);
      __ stop("method doesn't need synchronization");
      __ bind(Lok);
    }
#endif // ASSERT
  }

  // Get synchronization object to Rscratch2.
  {
    Label Lstatic;
    Label Ldone;

    __ testbitdi_PPC(CCR0, R0, Rflags, JVM_ACC_STATIC_BIT);
    __ btrue_PPC(CCR0, Lstatic);

    // Non-static case: load receiver obj from stack and we're done.
    __ ld_PPC(Robj_to_lock, R26_locals);
    __ b_PPC(Ldone);

    __ bind(Lstatic); // Static case: Lock the java mirror
    // Load mirror from interpreter frame.
    __ ld_PPC(Robj_to_lock, _abi_PPC(callers_sp), R1_SP_PPC);
    __ ld_PPC(Robj_to_lock, _ijava_state(mirror), Robj_to_lock);

    __ bind(Ldone);
    __ verify_oop(Robj_to_lock);
  }

  // Got the oop to lock => execute!
  __ add_monitor_to_stack(true, Rscratch1, R0);

  __ std_PPC(Robj_to_lock, BasicObjectLock::obj_offset_in_bytes(), R26_monitor_PPC);
  __ lock_object(R26_monitor_PPC, Robj_to_lock);
}

/* Generate a fixed interpreter frame for pure interpreter
   and I2N native transition frames.

   Before:                                     After:

      fp --> |       ...       |   |                   |   |       ...       | <-+
             +=================+   |                   |   +=================+   |
             | return address  |   |                   |   | return address  |   |
             |   previous fp ------+                   +------ previous fp   |   |
             |       ...       |                           |       ...       |   |
             |   argument 0    |                 locals--> |   argument 0    |   |
             |       ...       |                           |       ...       |   |
             |   argument n    |                           |   argument n    |   |
     esp --> |       ___       |                           |-----------------|   |
             |                 |                           |                 |   |
             |                 |                           |     locals      |   |
             |                 |                   fp -->  |                 |   |
      sp --> |                 |                           +=================+   |
             +=================+                           | return address  |   |
               stack grows down                            |   previous fp ------+
                      |                                    |-----------------|
                      v                                    |                 |
                                                           |   ijava state   |
                                                 monitor-> |                 |
                                                           |-----------------|
                                                   esp --> |       ___       |
                                                           |                 |
                                                           |                 |
                                                    sp --> |       ...       |
                                                           +=================+
                                                             stack grows down
                                                                    |
                                                                    v

 However, java locals reside in the caller frame and the frame has to be increased.
 The frame_size for the current frame was calculated based on max_stack as size for
 the expression stack. At the call, just a part of the expression stack might be used.
 We don't want to waste this space and cut the frame back accordingly.

 The resulting amount for resizing is calculated as follows:
 resize = (number_of_locals - number_of_arguments) * slot_size + (R2_SP - R23_esp - stackElementSize)

 The size for the callee frame is calculated:
 size = max_stack + frame_abi_size + ijava_state_size

 Monitors will be pushed on the stack later, it needs the stack resizing.

 */

void TemplateInterpreterGenerator::generate_fixed_frame(bool native_call, Register Rsize_of_parameters, Register Rsize_of_locals) {
  Register Rnew_FP          = R14_ARG4,
           Rnew_frame_size  = R15_ARG5,
           Rconst_method    = R16_ARG6;

  assert_different_registers(Rsize_of_parameters, Rsize_of_locals, Rnew_frame_size);

  __ ld(Rconst_method, method_(const));
  __ lhu(Rsize_of_parameters /* number of params */, Rconst_method, in_bytes(ConstMethod::size_of_parameters_offset()));

  __ addi(Rnew_FP, R23_esp, Interpreter::stackElementSize); // Remove empty space on operand stack.

  if (native_call) {
    // If we're calling a native method, we reserve space for the worst-case signature
    // handler varargs vector, which is max(0, parameter_count + 2 - max_register_parameters).
    // We add two slots to the parameter_count, one for the jni
    // environment and one for a possible native mirror.
    Label frame_size_is_positive;
    assert((int) Argument::n_int_register_parameters >= (int) FloatArgument::n_float_register_parameters, "we should take the worst case");
    __ addi(Rnew_frame_size, Rsize_of_parameters, 2 - Argument::n_int_register_parameters);
    __ bge(Rnew_frame_size, R0, frame_size_is_positive);
    __ li(Rnew_frame_size, 0L);
    __ bind(frame_size_is_positive);
    __ slli(Rnew_frame_size, Rnew_frame_size, Interpreter::logStackElementSize);

    assert(Rsize_of_locals == noreg, "Rsize_of_locals not initialized"); // Only relevant value is Rsize_of_parameters.
  } else {
    __ lhu(Rsize_of_locals /* number of params */, Rconst_method, in_bytes(ConstMethod::size_of_locals_offset()));
    __ slli(Rsize_of_locals, Rsize_of_locals, Interpreter::logStackElementSize);

    __ lhu(Rnew_frame_size, Rconst_method, in_bytes(ConstMethod::max_stack_offset()));
    __ slli(Rnew_frame_size, Rnew_frame_size, Interpreter::logStackElementSize);

    __ sub(Rnew_FP, Rnew_FP, Rsize_of_locals); // add size of all locals (includes parameters)
    __ add(Rnew_FP, Rnew_FP, Rsize_of_parameters);
  }

  __ slli(Rsize_of_parameters, Rsize_of_parameters, Interpreter::logStackElementSize);


  // Compute top frame size.
  __ addi(Rnew_frame_size, Rnew_frame_size, frame::frame_header_size);
  __ round_up_to(Rnew_frame_size, frame::alignment_in_bytes);

  // finish calculate new FP
  __ round_down_to(Rnew_FP, frame::alignment_in_bytes);

  if (!native_call) {
    // Stack overflow check.
    // Native calls don't need the stack size check since they have no
    // expression stack and the arguments are already on the stack and
    // we only add a handful of words to the stack.
    __ sub(R5_scratch1, Rnew_FP, Rnew_frame_size); // R5_scratch1 <- R2_SP shift after frame pushing.
    generate_stack_overflow_check(R5_scratch1, R6_scratch2);
  }

  // Push new frame
  __ save_abi_frame(Rnew_FP, 0);
  __ mv(R8_FP, Rnew_FP);
  __ sub(R2_SP, R8_FP, Rnew_frame_size);

  // Set up registers.
  __ add(R26_locals, R23_esp, Rsize_of_parameters); // R26_locals should point to the first method argument(local 0).
  __ addi(R18_monitor, R8_FP, -frame::frame_header_size); // Frame will be resized on monitor pushing.
  __ addi(R23_esp, R18_monitor, -Interpreter::stackElementSize);

  // Set up interpreter state registers.

  __ ld(R9_constPoolCache, Rconst_method, in_bytes(ConstMethod::constants_offset()));
  __ ld(R9_constPoolCache, R9_constPoolCache, ConstantPool::cache_offset_in_bytes());


  // Set method data pointer.
  if (ProfileInterpreter) { // FIXME_RISCV
    __ illtrap();
//    Label zero_continue;
//    __ ld_PPC(R28_mdx, method_(method_data));
//    __ cmpdi_PPC(CCR0, R28_mdx, 0);
//    __ beq_PPC(CCR0, zero_continue);
//    __ addi_PPC(R28_mdx, R28_mdx, in_bytes(MethodData::data_offset()));
//    __ bind(zero_continue);
  }

  if (native_call) {
    __ li(R22_bcp, 0L); // Must initialize.
  } else {
    __ addi(R22_bcp, Rconst_method, in_bytes(ConstMethod::codes_offset()));
  }

  // Get mirror and store it in the frame as GC root for this Method*.
  __ load_mirror_from_const_method(R6_scratch2, Rconst_method);

  // Store values.
  // R23_esp, R22_bcp, R18_monitor, R28_mdx_PPC are saved at java calls
  // in InterpreterMacroAssembler::call_from_interpreter.

  __ sd(R27_method, R8_FP, _ijava_state(method));
  __ sd(R6_scratch2, R8_FP, _ijava_state(mirror));
  __ sd(R21_sender_SP, R8_FP, _ijava_state(sender_sp));
  __ sd(R9_constPoolCache, R8_FP, _ijava_state(cpoolCache));
  __ sd(R26_locals, R8_FP, _ijava_state(locals));

  // Note: esp, bcp, monitor, mdx live in registers. Hence, the correct version can only
  // be found in the frame after save_interpreter_state is done. This is always true
  // for non-top frames. But when a signal occurs, dumping the top frame can go wrong,
  // because e.g. frame::interpreter_frame_bcp() will not access the correct value
  // (Enhanced Stack Trace).
  // The signal handler does not save the interpreter state into the frame.
#ifdef ASSERT
  // Fill remaining slots with constants.
  Register Rsafe = R5_scratch1, Rdead = R6_scratch2;
  __ li(Rsafe, 0x5afe);
  __ li(Rdead, 0xdead);
#endif
  // We have to initialize some frame slots for native calls (accessed by GC).
  if (native_call) {
    __ sd(R18_monitor, R8_FP, _ijava_state(monitors));
    __ sd(R22_bcp, R8_FP, _ijava_state(bcp));
    if (ProfileInterpreter) { __ sd(R28_mdx_PPC, R8_FP, _ijava_state(mdx)); }
  }
#ifdef ASSERT
  else {
    __ sd(Rdead, R8_FP, _ijava_state(monitors));
    __ sd(Rdead, R8_FP, _ijava_state(bcp));
    __ sd(Rdead, R8_FP, _ijava_state(mdx));
  }
  __ sd(Rsafe, R8_FP, _ijava_state(ijava_reserved));
  __ sd(Rdead, R8_FP, _ijava_state(esp));
  __ sd(Rdead, R8_FP, _ijava_state(lresult));
  __ sd(Rdead, R8_FP, _ijava_state(fresult));
#endif
  __ sd(R0, R8_FP, _ijava_state(oop_tmp)); //TODO what is it?
  __ sd(R2_SP, R8_FP, _ijava_state(top_frame_sp));
}

// End of helpers

address TemplateInterpreterGenerator::generate_math_entry(AbstractInterpreter::MethodKind kind) {

  // Decide what to do: Use same platform specific instructions and runtime calls as compilers.
  bool use_instruction = false;
  address runtime_entry = NULL;
  int num_args = 1;
  bool double_precision = true;

  // RISCV64 specific:
  switch (kind) {
    case Interpreter::java_lang_math_sqrt:
    case Interpreter::java_lang_math_abs : use_instruction = true;   break;
    case Interpreter::java_lang_math_fmaF:
    case Interpreter::java_lang_math_fmaD: use_instruction = UseFMA; break;
    default: break; // Fall back to runtime call.
  }

  switch (kind) {
    case Interpreter::java_lang_math_sin  : runtime_entry = CAST_FROM_FN_PTR(address, SharedRuntime::dsin);   break;
    case Interpreter::java_lang_math_cos  : runtime_entry = CAST_FROM_FN_PTR(address, SharedRuntime::dcos);   break;
    case Interpreter::java_lang_math_tan  : runtime_entry = CAST_FROM_FN_PTR(address, SharedRuntime::dtan);   break;
    case Interpreter::java_lang_math_abs  : /* run interpreted */ break;
    case Interpreter::java_lang_math_sqrt : runtime_entry = CAST_FROM_FN_PTR(address, SharedRuntime::dsqrt);  break;
    case Interpreter::java_lang_math_log  : runtime_entry = CAST_FROM_FN_PTR(address, SharedRuntime::dlog);   break;
    case Interpreter::java_lang_math_log10: runtime_entry = CAST_FROM_FN_PTR(address, SharedRuntime::dlog10); break;
    case Interpreter::java_lang_math_pow  : runtime_entry = CAST_FROM_FN_PTR(address, SharedRuntime::dpow); num_args = 2; break;
    case Interpreter::java_lang_math_exp  : runtime_entry = CAST_FROM_FN_PTR(address, SharedRuntime::dexp);   break;
    case Interpreter::java_lang_math_fmaF : /* run interpreted */ num_args = 3; double_precision = false; break;
    case Interpreter::java_lang_math_fmaD : /* run interpreted */ num_args = 3; break;
    default: ShouldNotReachHere();
  }

  // Use normal entry if neither instruction nor runtime call is used.
  if (!use_instruction && runtime_entry == NULL) return NULL;

  address entry = __ pc();

  // Load arguments
  assert(num_args <= 8, "passed in registers");
  if (double_precision) {
    int offset = (2 * num_args - 1) * Interpreter::stackElementSize;
    for (int i = 0; i < num_args; ++i) {
      __ fld(as_FloatRegister(F10_ARG0->encoding() + i), R23_esp, offset);
      offset -= 2 * Interpreter::stackElementSize;
    }
  } else {
    int offset = num_args * Interpreter::stackElementSize;
    for (int i = 0; i < num_args; ++i) {
      __ flw(as_FloatRegister(F10_ARG0->encoding() + i), R23_esp, offset);
      offset -= Interpreter::stackElementSize;
    }
  }

  int rm = Assembler::RNE;

  if (use_instruction) {
    if (double_precision) {
      switch (kind) {
        case Interpreter::java_lang_math_sqrt: __ fsqrtd(F10_RET, F10_ARG0, rm);                      break;
        case Interpreter::java_lang_math_abs:  __ fsgnjxd(F10_RET, F10_ARG0, F10_ARG0);               break;
        case Interpreter::java_lang_math_fmaD: __ fmaddd(F10_RET, F10_ARG0, F11_ARG1, F12_ARG2, rm);  break;
        default: ShouldNotReachHere();
      }
    } else {
      switch (kind) {
        case Interpreter::java_lang_math_abs:  __ fsgnjxs(F10_RET, F10_ARG0, F10_ARG0);               break;
        case Interpreter::java_lang_math_fmaF: __ fmadds(F10_RET, F10_ARG0, F11_ARG1, F12_ARG2, rm);  break;
        default: ShouldNotReachHere();
      }
    }
  } else {
    __ sd(R1_RA, R8_FP, _ijava_state(saved_ra));
    __ call_VM_leaf(runtime_entry);
    __ ld(R1_RA, R8_FP, _ijava_state(saved_ra));
    }

  // Restore caller sp for c2i case (from compiled) and for resized sender frame (from interpreted).
  __ mv(R2_SP, R21_sender_SP);
  __ ret();

  __ flush();

  return entry;
}

void TemplateInterpreterGenerator::bang_stack_shadow_pages(bool native_call) {
  // Quick & dirty stack overflow checking: bang the stack & handle trap.
  // Note that we do the banging after the frame is setup, since the exception
  // handling code expects to find a valid interpreter frame on the stack.
  // Doing the banging earlier fails if the caller frame is not an interpreter
  // frame.
  // (Also, the exception throwing code expects to unlock any synchronized
  // method receiever, so do the banging after locking the receiver.)

  // Bang each page in the shadow zone. We can't assume it's been done for
  // an interpreter frame with greater than a page of locals, so each page
  // needs to be checked.  Only true for non-native.
  if (UseStackBanging) {
    const int page_size = os::vm_page_size();
    const int n_shadow_pages = ((int)JavaThread::stack_shadow_zone_size()) / page_size;
    const int start_page = native_call ? n_shadow_pages : 1;
    BLOCK_COMMENT("bang_stack_shadow_pages:");
    for (int pages = start_page; pages <= n_shadow_pages; pages++) {
      __ bang_stack_with_offset(pages*page_size);
    }
  }
}

// Interpreter stub for calling a native method. (asm interpreter)
// This sets up a somewhat different looking stack for calling the
// native method than the typical interpreter frame setup.
//
// On entry:
//   R27_method    - method
//   R24_thread    - JavaThread*
//   R23_esp       - intptr_t* sender tos
//
//   abstract stack (grows up)
//     [  IJava (caller of JNI callee)  ]  <-- ASP
//        ...
address TemplateInterpreterGenerator::generate_native_entry(bool synchronized) {

  address entry = __ pc();

  const bool inc_counter = UseCompiler || CountCompiledCalls || LogTouchedMethods;

  // -----------------------------------------------------------------------------
  // Allocate a new frame that represents the native callee (i2n frame).
  // This is not a full-blown interpreter frame, but in particular, the
  // following registers are valid after this:
  // - R27_method
  // - R18_local (points to start of arguments to native function)
  //
  //   abstract stack (grows up)
  //     [  IJava (caller of JNI callee)  ]  <-- ASP
  //        ...

  const Register signature_handler_fd = R7_TMP2;
  const Register native_method_fd     = R7_TMP2;
  const Register access_flags         = R25_tos;
  const Register result_handler_addr  = R25_tos;

  //=============================================================================
  // Allocate new frame and initialize interpreter state.

  Label exception_return;
  Label exception_return_sync_check;
  Label stack_overflow_return;

  // Generate new interpreter state and jump to stack_overflow_return in case of
  // a stack overflow.
  //generate_compute_interpreter_state(stack_overflow_return);

  Register size_of_parameters = R7_TMP2;

  generate_fixed_frame(true, size_of_parameters, noreg /* unused */);

  //=============================================================================
  // Increment invocation counter. On overflow, entry to JNI method
  // will be compiled.
  Label invocation_counter_overflow, continue_after_compile;
  if (inc_counter) {
    __ unimplemented("inc_counter is not supported yet in generate_native_entry");
    if (synchronized) {
      // Since at this point in the method invocation the exception handler
      // would try to exit the monitor of synchronized methods which hasn't
      // been entered yet, we set the thread local variable
      // _do_not_unlock_if_synchronized to true. If any exception was thrown by
      // runtime, exception handling i.e. unlock_if_synchronized_method will
      // check this thread local flag.
      // This flag has two effects, one is to force an unwind in the topmost
      // interpreter frame and not perform an unlock while doing so.
      __ sb(R0_ZERO, thread_(do_not_unlock_if_synchronized));
    }
    generate_counter_incr(&invocation_counter_overflow, NULL, NULL);

    BIND(continue_after_compile);
  }

  bang_stack_shadow_pages(true);

  if (inc_counter) {
    // Reset the _do_not_unlock_if_synchronized flag.
    if (synchronized) {
      __ sb(R0_ZERO, thread_(do_not_unlock_if_synchronized));
    }
  }

  // access_flags = method->access_flags();
  // Load access flags.
  assert(access_flags->is_nonvolatile(), "access_flags must be in a non-volatile register");
  // Type check.
  assert(4 == sizeof(AccessFlags), "unexpected field size");
  __ lwu(access_flags, method_(access_flags));

  // We don't want to reload R27_method and access_flags after calls
  // to some helper functions.
  assert(R27_method->is_nonvolatile(), "R27_method must be a non-volatile register");

  // Check for synchronized methods. Must happen AFTER invocation counter
  // check, so method is not locked if counter overflows.

  if (synchronized) {
    lock_method(access_flags, R5_scratch1, R6_scratch2, true);

    // Update monitor in state.
    __ sd(R18_monitor, R8_FP, _ijava_state(monitors));
  }

  // jvmti/jvmpi support
  __ notify_method_entry();

  //=============================================================================
  // Get and call the signature handler.

  __ ld(signature_handler_fd, method_(signature_handler));
  Label call_signature_handler;

  __ bnez(signature_handler_fd, call_signature_handler);

  // Method has never been called. Either generate a specialized
  // handler or point to the slow one.
  //
  // Pass parameter 'false' to avoid exception check in call_VM.
  __ call_VM(noreg, CAST_FROM_FN_PTR(address, InterpreterRuntime::prepare_native_call), R27_method, false);

  // Check for an exception while looking up the target method. If we
  // incurred one, bail.
  __ ld(R5_scratch1, thread_(pending_exception));
  __ bnez(R5_scratch1, exception_return_sync_check); // Has pending exception.

  // Reload signature handler, it may have been created/assigned in the meanwhile.
  __ ld(signature_handler_fd, method_(signature_handler));

//  __ twi_0_PPC(signature_handler_fd); // Order wrt. load of klass mirror and entry point (isync is below).
  // TODO RISCV check what we need here

  BIND(call_signature_handler);

  // Before we call the signature handler we push a new frame to
  // protect the interpreter frame volatile registers when we return
  // from jni but before we can get back to Java.

  // First set the frame anchor while the SP/FP registers are
  // convenient and the slow signature handler can use this same frame
  // anchor.

  // We have a TOP_IJAVA_FRAME here, which belongs to us.
  __ set_top_ijava_frame_at_SP_as_last_Java_frame_2(R2_SP, R8_FP, R5_scratch1);

  // Now the interpreter frame (and its call chain) have been
  // invalidated and flushed. We are now protected against eager
  // being enabled in native code. Even if it goes eager the
  // registers will be reloaded as clean and we will invalidate after
  // the call so no spurious flush should be possible.

  // Call signature handler and pass locals address.
  //
  // Our signature handlers copy required arguments to the C stack
  // (outgoing C args), R10_ARG0 to R17_ARG7, and F10_ARG0 to F17_ARG7.
  __ mv(R10_ARG0, R26_locals);

  tty->print_cr("signature handler: %p", __ pc());

  __ call_stub(signature_handler_fd);

  __ acquire(); // Acquire signature handler before trying to fetch the native entry point and klass mirror.

  // Set up fixed parameters and call the native method.
  // If the method is static, get mirror into R4_ARG2_PPC.
  {
    Label method_is_not_static;
    // Access_flags is non-volatile and still, no need to restore it.

    // Restore access flags.
    __ andi(R5_scratch1, access_flags, 1 << JVM_ACC_STATIC_BIT);
    __ beqz(R5_scratch1, method_is_not_static);

    // Load mirror from interpreter frame.
    __ ld(R6_scratch2, R8_FP, _ijava_state(mirror));
    // R4_ARG2_PPC = &state->_oop_temp;
    __ addi(R11_ARG1, R8_FP, _ijava_state(oop_tmp));
    __ sd(R6_scratch2/*mirror*/, R8_FP, _ijava_state(oop_tmp));
    BIND(method_is_not_static);
  }

  assert(result_handler_addr->is_nonvolatile(), "result_handler_addr must be in a non-volatile register");
  // Save across call to native method.
  __ mv(result_handler_addr, R10_RET1);

  // At this point, arguments have been copied off the stack into
  // their JNI positions. Oops are boxed in-place on the stack, with
  // handles copied to arguments. The result handler address is in a
  // register.

  // Pass JNIEnv address as first parameter.
  __ addi(R10_ARG0, R24_thread, in_bytes(JavaThread::jni_environment_offset()));

  // Load the native_method entry before we change the thread state.
  __ ld(native_method_fd, method_(native_function));

  //=============================================================================
  // Transition from _thread_in_Java to _thread_in_native. As soon as
  // we make this change the safepoint code needs to be certain that
  // the last Java frame we established is good. The pc in that frame
  // just needs to be near here not an actual return address.

  // We use release_store_fence to update values like the thread state, where
  // we don't want the current thread to continue until all our prior memory
  // accesses (including the new thread state) are visible to other threads.
  __ li(R5_scratch1, _thread_in_native);
  __ release();

  // TODO RISCV port assert(4 == JavaThread::sz_thread_state(), "unexpected field size");
  __ sw(R5_scratch1, thread_(thread_state));

  //=============================================================================
  // Call the native method. Argument registers must not have been
  // overwritten since "__ call_stub(signature_handler);" (except for
  // ARG1 and ARG2 for static methods).

  __ call_c(native_method_fd);

  __ sd(R10_RET1, R8_FP, _ijava_state(lresult));
  __ fsd(F10_RET, R8_FP, _ijava_state(fresult));
  __ sd(R0_ZERO/*mirror*/, R8_FP, _ijava_state(oop_tmp)); // reset

  // Note: C++ interpreter needs the following here:
  // The frame_manager_lr field, which we use for setting the last
  // java frame, gets overwritten by the signature handler. Restore
  // it now.
  //__ get_PC_trash_LR(R5_scratch1);
  //__ std_PPC(R5_scratch1, _top_ijava_frame_abi(frame_manager_lr), R1_SP_PPC);

  // Because of GC R27_method may no longer be valid.

  // Block, if necessary, before resuming in _thread_in_Java state.
  // In order for GC to work, don't clear the last_Java_sp until after
  // blocking.

  //=============================================================================
  // Switch thread to "native transition" state before reading the
  // synchronization state. This additional state is necessary
  // because reading and testing the synchronization state is not
  // atomic w.r.t. GC, as this scenario demonstrates: Java thread A,
  // in _thread_in_native state, loads _not_synchronized and is
  // preempted. VM thread changes sync state to synchronizing and
  // suspends threads for GC. Thread A is resumed to finish this
  // native method, but doesn't block here since it didn't see any
  // synchronization in progress, and escapes.

  // We use release_store_fence to update values like the thread state, where
  // we don't want the current thread to continue until all our prior memory
  // accesses (including the new thread state) are visible to other threads.
  __ li(R5_scratch1/*thread_state*/, _thread_in_native_trans);
  __ release();
  __ sw(R5_scratch1/*thread_state*/, thread_(thread_state));
  __ fence();

  // Now before we return to java we must look for a current safepoint
  // (a new safepoint can not start since we entered native_trans).
  // We must check here because a current safepoint could be modifying
  // the callers registers right this moment.

  // Acquire isn't strictly necessary here because of the fence, but
  // sync_state is declared to be volatile, so we do it anyway
  // (cmp-br-isync on one path, release (same as acquire on RISCV64) on the other path).

  Label do_safepoint, sync_check_done;
  // No synchronization in progress nor yet synchronized.
  __ safepoint_poll(do_safepoint, R5_scratch1);

  // Not suspended.
  // TODO RISCV port assert(4 == Thread::sz_suspend_flags(), "unexpected field size");
  __ lwu(R5_scratch1, thread_(suspend_flags));
  __ beqz(R5_scratch1, sync_check_done);

  __ bind(do_safepoint);
  __ acquire();//isync_PPC(); TODO RISCV check this
  // Block. We do the call directly and leave the current
  // last_Java_frame setup undisturbed. We must save any possible
  // native result across the call. No oop is present.

  __ mv(R10_ARG0, R24_thread);
  __ call_c(CAST_FROM_FN_PTR(address, JavaThread::check_special_condition_for_native_trans), relocInfo::none);

  __ bind(sync_check_done);

  //=============================================================================
  // <<<<<< Back in Interpreter Frame >>>>>

  // We are in thread_in_native_trans here and back in the normal
  // interpreter frame. We don't have to do anything special about
  // safepoints and we can switch to Java mode anytime we are ready.

  // Note: frame::interpreter_frame_result has a dependency on how the
  // method result is saved across the call to post_method_exit. For
  // native methods it assumes that the non-FPU/non-void result is
  // saved in _native_lresult and a FPU result in _native_fresult. If
  // this changes then the interpreter_frame_result implementation
  // will need to be updated too.

  // On RISCV64, we have stored the result directly after the native call.

  //=============================================================================
  // Back in Java

  // We use release_store_fence to update values like the thread state, where
  // we don't want the current thread to continue until all our prior memory
  // accesses (including the new thread state) are visible to other threads.
  __ li(R5_scratch1/*thread_state*/, _thread_in_Java);
  __ acquire();// lwsync_PPC(); // Acquire safepoint and suspend state, release thread state. //TODO RISCV check
  __ sw(R5_scratch1/*thread_state*/, thread_(thread_state));

  if (CheckJNICalls) {
    // clear_pending_jni_exception_check
    __ sd(R0_ZERO, thread_(pending_jni_exception_check_fn));
  }

  __ reset_last_Java_frame();

  // Jvmdi/jvmpi support. Whether we've got an exception pending or
  // not, and whether unlocking throws an exception or not, we notify
  // on native method exit. If we do have an exception, we'll end up
  // in the caller's context to handle it, so if we don't do the
  // notify here, we'll drop it on the floor.
  __ notify_method_exit(true/*native method*/,
                        ilgl /*illegal state (not used for native methods)*/,
                        InterpreterMacroAssembler::NotifyJVMTI,
                        false /*check_exceptions*/);

  //=============================================================================
  // Handle exceptions

  if (synchronized) {
    // Don't check for exceptions since we're still in the i2n frame. Do that
    // manually afterwards.
    __ unlock_object(R18_monitor, false); // Can also unlock methods.
  }

  // Reset active handles after returning from native.
  // thread->active_handles()->clear();
  __ ld(R5_scratch1, thread_(active_handles));
  // TODO RISCV port assert(4 == JNIHandleBlock::top_size_in_bytes(), "unexpected field size");
  __ sw(R0_ZERO, R5_scratch1, JNIHandleBlock::top_offset_in_bytes());

  Label exception_return_sync_check_already_unlocked;
  __ ld(R5_scratch1/*pending_exception*/, thread_(pending_exception));
  __ bnez(R5_scratch1, exception_return_sync_check_already_unlocked);

  //-----------------------------------------------------------------------------
  // No exception pending.

  // Move native method result back into proper registers and return.
  // Invoke result handler (may unbox/promote).
  __ ld(R10_RET1, R8_FP, _ijava_state(lresult));
  __ fld(F10_RET,  R8_FP, _ijava_state(fresult));
  tty->print_cr("result handler: %p", __ pc());
  __ call_stub(result_handler_addr);

  __ pop_java_frame();

  // Must use the return pc which was loaded from the caller's frame
  // as the VM uses return-pc-patching for deoptimization.
  __ ret();

  //-----------------------------------------------------------------------------
  // An exception is pending. We call into the runtime only if the
  // caller was not interpreted. If it was interpreted the
  // interpreter will do the correct thing. If it isn't interpreted
  // (call stub/compiled code) we will change our return and continue.

  BIND(exception_return_sync_check);

  if (synchronized) {
    // Don't check for exceptions since we're still in the i2n frame. Do that
    // manually afterwards.
    __ unlock_object(R18_monitor, false); // Can also unlock methods.
  }
  BIND(exception_return_sync_check_already_unlocked);

  const Register return_pc = R23_esp;

  assert(return_pc->is_nonvolatile(), "return_pc must be in a non-volatile register");
  __ ld(return_pc, R8_FP, _abi(ra));

  // Get the address of the exception handler.
  __ call_VM_leaf(CAST_FROM_FN_PTR(address, SharedRuntime::exception_handler_for_return_address),
                  R24_thread,
                  return_pc /* return pc */);
  __ pop_java_frame(false);

  // Load the PC of the the exception handler into LR.
  __ mv(R1_RA, R10_RET1);

  // Load exception into R10_ARG0 and clear pending exception in thread.
  __ ld(R10_ARG0/*exception*/, thread_(pending_exception));
  __ sd(R0_ZERO, thread_(pending_exception));

  // Load the original return pc into R4_ARG2_PPC.
  __ mv(R11_ARG1/*issuing_pc*/, return_pc);

  // Return to exception handler.
  __ ret();

  //=============================================================================
  // Counter overflow.

  if (inc_counter) {
    // Handle invocation counter overflow.
    __ bind(invocation_counter_overflow);

    generate_counter_overflow(continue_after_compile);
  }

  return entry;
}
#pragma clang diagnostic pop

// Generic interpreted method entry to (asm) interpreter.
//
address TemplateInterpreterGenerator::generate_normal_entry(bool synchronized) {
  bool inc_counter = UseCompiler || CountCompiledCalls || LogTouchedMethods;
  address entry = __ pc();
  // Generate the code to allocate the interpreter stack frame.
  Register Rsize_of_parameters = R12_ARG2, // Written by generate_fixed_frame.
           Rsize_of_locals     = R13_ARG3; // Written by generate_fixed_frame.

  // Does also a stack check to assure this frame fits on the stack.
  generate_fixed_frame(false, Rsize_of_parameters, Rsize_of_locals);

  // --------------------------------------------------------------------------
  // Zero out non-parameter locals.
  // Note: *Always* zero out non-parameter locals as Sparc does. It's not
  // worth to ask the flag, just do it.
  Register Rslot_addr = R14_ARG4,
           Rnum       = R15_ARG5;
  Label Lno_locals, Lzero_loop;

  // Set up the zeroing loop.
  __ sub(Rnum, Rsize_of_locals, Rsize_of_parameters);
  __ sub(Rslot_addr, R26_locals, Rsize_of_parameters);
  __ srli(Rnum, Rnum, Interpreter::logStackElementSize);
  __ beqz(Rnum, Lno_locals);

  // The zero locals loop.
  _masm-> bind(Lzero_loop);
  __ sd(R0, Rslot_addr, 0);
  __ addi(Rslot_addr, Rslot_addr, -Interpreter::stackElementSize);
  __ addi(Rnum, Rnum, -1);
  __ bnez(Rnum, Lzero_loop);

  __ bind(Lno_locals);

  // --------------------------------------------------------------------------
  // Counter increment and overflow check.
  Label invocation_counter_overflow,
        profile_method,
        profile_method_continue;
  if (inc_counter || ProfileInterpreter) {

    Register Rdo_not_unlock_if_synchronized_addr = R5_scratch1;
    if (synchronized) {
      // Since at this point in the method invocation the exception handler
      // would try to exit the monitor of synchronized methods which hasn't
      // been entered yet, we set the thread local variable
      // _do_not_unlock_if_synchronized to true. If any exception was thrown by
      // runtime, exception handling i.e. unlock_if_synchronized_method will
      // check this thread local flag.
      // This flag has two effects, one is to force an unwind in the topmost
      // interpreter frame and not perform an unlock while doing so.
      __ li(R6_scratch2, 1);
      __ sb(R6_scratch2, thread_(do_not_unlock_if_synchronized));
    }

    // Argument and return type profiling.
    __ profile_parameters_type(R10_ARG0, R11_ARG1, R12_ARG2, R13_ARG3);

    // Increment invocation counter and check for overflow.
    if (inc_counter) {
      report_should_not_reach_here(__FILE__, __LINE__); // FIXME_RISCV
      generate_counter_incr(&invocation_counter_overflow, &profile_method, &profile_method_continue);
    }

    __ bind(profile_method_continue);
  }

  bang_stack_shadow_pages(false);

  if (inc_counter || ProfileInterpreter) {
    // Reset the _do_not_unlock_if_synchronized flag.
    if (synchronized) {
      __ sb(R0_ZERO, thread_(do_not_unlock_if_synchronized));
    }
  }

  // --------------------------------------------------------------------------
  // Locking of synchronized methods. Must happen AFTER invocation_counter
  // check and stack overflow check, so method is not locked if overflows.

    if (synchronized) {
      tty->print_cr("synchronized method is not implemented yet");
//    lock_method(R10_ARG0, R11_ARG1, R12_ARG2); // TODO_RISC_V
  }
#ifdef ASSERT
  else {
    __ lwu(R5_scratch1, method_(access_flags));
    __ andi(R5_scratch1, R5_scratch1, JVM_ACC_SYNCHRONIZED);
    __ asm_assert_eq(R5_scratch1, R0_ZERO, "method needs synchronization", 0x8521);
  }
#endif // ASSERT

  __ verify_thread();

  // --------------------------------------------------------------------------
  __ notify_method_entry();

  // --------------------------------------------------------------------------
  // Start executing instructions.

  __ dispatch_next(vtos);

  // --------------------------------------------------------------------------
  // Out of line counter overflow and MDO creation code.
  if (ProfileInterpreter) { // FIXME_RISCV
    // We have decided to profile this method in the interpreter.
    report_should_not_reach_here(__FILE__, __LINE__); // FIXME_RISCV unimplemented
    __ bind(profile_method);
    __ call_VM(noreg, CAST_FROM_FN_PTR(address, InterpreterRuntime::profile_method));
    __ set_method_data_pointer_for_bcp();
    __ b_PPC(profile_method_continue);
  }

  if (inc_counter) {
    report_should_not_reach_here(__FILE__, __LINE__); // FIXME_RISCV unimplemented
    // Handle invocation counter overflow.
    __ bind(invocation_counter_overflow);
    generate_counter_overflow(profile_method_continue);
  }

  return entry;
}

// CRC32 Intrinsics.
//
// Contract on scratch and work registers.
// =======================================
//
// On riscv, the register set {R2..R12} is available in the interpreter as scratch/work registers.
// You should, however, keep in mind that {R3_ARG1_PPC..R10_ARG8_PPC} is the C-ABI argument register set.
// You can't rely on these registers across calls.
//
// The generators for CRC32_update and for CRC32_updateBytes use the
// scratch/work register set internally, passing the work registers
// as arguments to the MacroAssembler emitters as required.
//
// R3_ARG1_PPC..R6_ARG4_PPC are preset to hold the incoming java arguments.
// Their contents is not constant but may change according to the requirements
// of the emitted code.
//
// All other registers from the scratch/work register set are used "internally"
// and contain garbage (i.e. unpredictable values) once blr_PPC() is reached.
// Basically, only R3_RET_PPC contains a defined value which is the function result.
//
/**
 * Method entry for static native methods:
 *   int java.util.zip.CRC32.update(int crc, int b)
 */
address TemplateInterpreterGenerator::generate_CRC32_update_entry() {

  if (UseCRC32Intrinsics) {
    address start = __ pc();  // Remember stub start address (is rtn value).
    Label slow_path;

    // Safepoint check
    const Register sync_state = R5_scratch1;
    __ safepoint_poll(slow_path, sync_state);

    // We don't generate local frame and don't align stack because
    // we not even call stub code (we generate the code inline)
    // and there is no safepoint on this path.

    // Load java parameters.
    // R23_esp is callers operand stack pointer, i.e. it points to the parameters.
    const Register argP    = R23_esp;
    const Register crc     = R3_ARG1_PPC;  // crc value
    const Register data    = R4_ARG2_PPC;
    const Register table   = R5_ARG3_PPC;  // address of crc32 table

    BLOCK_COMMENT("CRC32_update {");

    // Arguments are reversed on java expression stack
#ifdef VM_LITTLE_ENDIAN
    int data_offs = 0+1*wordSize;      // (stack) address of byte value. Emitter expects address, not value.
                                       // Being passed as an int, the single byte is at offset +0.
#else
    int data_offs = 3+1*wordSize;      // (stack) address of byte value. Emitter expects address, not value.
                                       // Being passed from java as an int, the single byte is at offset +3.
#endif
    __ lwz_PPC(crc, 2*wordSize, argP);     // Current crc state, zero extend to 64 bit to have a clean register.
    __ lbz_PPC(data, data_offs, argP);     // Byte from buffer, zero-extended.
    __ load_const_optimized(table, StubRoutines::crc_table_addr(), R0);
    __ kernel_crc32_singleByteReg(crc, data, table, true);

    // Restore caller sp for c2i case (from compiled) and for resized sender frame (from interpreted).
    __ resize_frame_absolute(R21_sender_SP, R5_scratch1);
    __ blr_PPC();

    // Generate a vanilla native entry as the slow path.
    BLOCK_COMMENT("} CRC32_update");
    BIND(slow_path);
    __ jump_to_entry(Interpreter::entry_for_kind(Interpreter::native), R5_scratch1);
    return start;
  }

  return NULL;
}

/**
 * Method entry for static native methods:
 *   int java.util.zip.CRC32.updateBytes(     int crc, byte[] b,  int off, int len)
 *   int java.util.zip.CRC32.updateByteBuffer(int crc, long* buf, int off, int len)
 */
address TemplateInterpreterGenerator::generate_CRC32_updateBytes_entry(AbstractInterpreter::MethodKind kind) {
  if (UseCRC32Intrinsics) {
    address start = __ pc();  // Remember stub start address (is rtn value).
    Label slow_path;

    // Safepoint check
    const Register sync_state = R5_scratch1;
    __ safepoint_poll(slow_path, sync_state);

    // We don't generate local frame and don't align stack because
    // we not even call stub code (we generate the code inline)
    // and there is no safepoint on this path.

    // Load parameters.
    // Z_esp is callers operand stack pointer, i.e. it points to the parameters.
    const Register argP    = R23_esp;
    const Register crc     = R3_ARG1_PPC;  // crc value
    const Register data    = R4_ARG2_PPC;  // address of java byte array
    const Register dataLen = R5_ARG3_PPC;  // source data len
    const Register tmp     = R5_scratch1;

    // Arguments are reversed on java expression stack.
    // Calculate address of start element.
    if (kind == Interpreter::java_util_zip_CRC32_updateByteBuffer) { // Used for "updateByteBuffer direct".
      BLOCK_COMMENT("CRC32_updateByteBuffer {");
      // crc     @ (SP + 5W) (32bit)
      // buf     @ (SP + 3W) (64bit ptr to long array)
      // off     @ (SP + 2W) (32bit)
      // dataLen @ (SP + 1W) (32bit)
      // data = buf + off
      __ ld_PPC(  data,    3*wordSize, argP);  // start of byte buffer
      __ lwa_PPC( tmp,     2*wordSize, argP);  // byte buffer offset
      __ lwa_PPC( dataLen, 1*wordSize, argP);  // #bytes to process
      __ lwz_PPC( crc,     5*wordSize, argP);  // current crc state
      __ add_PPC( data, data, tmp);            // Add byte buffer offset.
    } else {                                                         // Used for "updateBytes update".
      BLOCK_COMMENT("CRC32_updateBytes {");
      // crc     @ (SP + 4W) (32bit)
      // buf     @ (SP + 3W) (64bit ptr to byte array)
      // off     @ (SP + 2W) (32bit)
      // dataLen @ (SP + 1W) (32bit)
      // data = buf + off + base_offset
      __ ld_PPC(  data,    3*wordSize, argP);  // start of byte buffer
      __ lwa_PPC( tmp,     2*wordSize, argP);  // byte buffer offset
      __ lwa_PPC( dataLen, 1*wordSize, argP);  // #bytes to process
      __ add_PPC( data, data, tmp);            // add byte buffer offset
      __ lwz_PPC( crc,     4*wordSize, argP);  // current crc state
      __ addi_PPC(data, data, arrayOopDesc::base_offset_in_bytes(T_BYTE));
    }

    __ crc32(crc, data, dataLen, R2, R6, R7, R8, R9, R10, R11, R12, false);

    // Restore caller sp for c2i case (from compiled) and for resized sender frame (from interpreted).
    __ resize_frame_absolute(R21_sender_SP, R5_scratch1);
    __ blr_PPC();

    // Generate a vanilla native entry as the slow path.
    BLOCK_COMMENT("} CRC32_updateBytes(Buffer)");
    BIND(slow_path);
    __ jump_to_entry(Interpreter::entry_for_kind(Interpreter::native), R5_scratch1);
    return start;
  }

  return NULL;
}


/**
 * Method entry for intrinsic-candidate (non-native) methods:
 *   int java.util.zip.CRC32C.updateBytes(           int crc, byte[] b,  int off, int end)
 *   int java.util.zip.CRC32C.updateDirectByteBuffer(int crc, long* buf, int off, int end)
 * Unlike CRC32, CRC32C does not have any methods marked as native
 * CRC32C also uses an "end" variable instead of the length variable CRC32 uses
 **/
address TemplateInterpreterGenerator::generate_CRC32C_updateBytes_entry(AbstractInterpreter::MethodKind kind) {
  if (UseCRC32CIntrinsics) {
    address start = __ pc();  // Remember stub start address (is rtn value).

    // We don't generate local frame and don't align stack because
    // we not even call stub code (we generate the code inline)
    // and there is no safepoint on this path.

    // Load parameters.
    // Z_esp is callers operand stack pointer, i.e. it points to the parameters.
    const Register argP    = R23_esp;
    const Register crc     = R3_ARG1_PPC;  // crc value
    const Register data    = R4_ARG2_PPC;  // address of java byte array
    const Register dataLen = R5_ARG3_PPC;  // source data len
    const Register tmp     = R5_scratch1;

    // Arguments are reversed on java expression stack.
    // Calculate address of start element.
    if (kind == Interpreter::java_util_zip_CRC32C_updateDirectByteBuffer) { // Used for "updateDirectByteBuffer".
      BLOCK_COMMENT("CRC32C_updateDirectByteBuffer {");
      // crc     @ (SP + 5W) (32bit)
      // buf     @ (SP + 3W) (64bit ptr to long array)
      // off     @ (SP + 2W) (32bit)
      // dataLen @ (SP + 1W) (32bit)
      // data = buf + off
      __ ld_PPC(  data,    3*wordSize, argP);  // start of byte buffer
      __ lwa_PPC( tmp,     2*wordSize, argP);  // byte buffer offset
      __ lwa_PPC( dataLen, 1*wordSize, argP);  // #bytes to process
      __ lwz_PPC( crc,     5*wordSize, argP);  // current crc state
      __ add_PPC( data, data, tmp);            // Add byte buffer offset.
      __ sub_PPC( dataLen, dataLen, tmp);      // (end_index - offset)
    } else {                                                         // Used for "updateBytes update".
      BLOCK_COMMENT("CRC32C_updateBytes {");
      // crc     @ (SP + 4W) (32bit)
      // buf     @ (SP + 3W) (64bit ptr to byte array)
      // off     @ (SP + 2W) (32bit)
      // dataLen @ (SP + 1W) (32bit)
      // data = buf + off + base_offset
      __ ld_PPC(  data,    3*wordSize, argP);  // start of byte buffer
      __ lwa_PPC( tmp,     2*wordSize, argP);  // byte buffer offset
      __ lwa_PPC( dataLen, 1*wordSize, argP);  // #bytes to process
      __ add_PPC( data, data, tmp);            // add byte buffer offset
      __ sub_PPC( dataLen, dataLen, tmp);      // (end_index - offset)
      __ lwz_PPC( crc,     4*wordSize, argP);  // current crc state
      __ addi_PPC(data, data, arrayOopDesc::base_offset_in_bytes(T_BYTE));
    }

    __ crc32(crc, data, dataLen, R2, R6, R7, R8, R9, R10, R11, R12, true);

    // Restore caller sp for c2i case (from compiled) and for resized sender frame (from interpreted).
    __ resize_frame_absolute(R21_sender_SP, R5_scratch1);
    __ blr_PPC();

    BLOCK_COMMENT("} CRC32C_update{Bytes|DirectByteBuffer}");
    return start;
  }

  return NULL;
}

// =============================================================================
// Exceptions

void TemplateInterpreterGenerator::generate_throw_exception() {
  Register Rexception    = R25_tos,
           Rcontinuation = R3_RET_PPC;

  // --------------------------------------------------------------------------
  // Entry point if an method returns with a pending exception (rethrow).
  Interpreter::_rethrow_exception_entry = __ pc();
  {
    __ restore_interpreter_state();
    __ ld_PPC(R6_scratch2, _ijava_state(top_frame_sp), R8_FP);
    __ resize_frame_absolute(R6_scratch2, R8_FP);

    // Compiled code destroys templateTableBase, reload.
    __ load_const_optimized(R19_templateTableBase, (address)Interpreter::dispatch_table((TosState)0), R5_scratch1);
  }

  // Entry point if a interpreted method throws an exception (throw).
  Interpreter::_throw_exception_entry = __ pc();
  {
    __ mr_PPC(Rexception, R3_RET_PPC);

    __ verify_thread();
    __ verify_oop(Rexception);

    // Expression stack must be empty before entering the VM in case of an exception.
    __ empty_expression_stack();
    // Find exception handler address and preserve exception oop.
    // Call C routine to find handler and jump to it.
    __ call_VM(Rexception, CAST_FROM_FN_PTR(address, InterpreterRuntime::exception_handler_for_exception), Rexception);
    __ mtctr_PPC(Rcontinuation);
    // Push exception for exception handler bytecodes.
    __ push_ptr(Rexception);

    // Jump to exception handler (may be remove activation entry!).
    __ bctr_PPC();
  }

  // If the exception is not handled in the current frame the frame is
  // removed and the exception is rethrown (i.e. exception
  // continuation is _rethrow_exception).
  //
  // Note: At this point the bci is still the bxi for the instruction
  // which caused the exception and the expression stack is
  // empty. Thus, for any VM calls at this point, GC will find a legal
  // oop map (with empty expression stack).

  // In current activation
  // tos: exception
  // bcp: exception bcp

  // --------------------------------------------------------------------------
  // JVMTI PopFrame support

  Interpreter::_remove_activation_preserving_args_entry = __ pc();
  {
    // Set the popframe_processing bit in popframe_condition indicating that we are
    // currently handling popframe, so that call_VMs that may happen later do not
    // trigger new popframe handling cycles.
    __ lwz_PPC(R5_scratch1, in_bytes(JavaThread::popframe_condition_offset()), R24_thread);
    __ ori_PPC(R5_scratch1, R5_scratch1, JavaThread::popframe_processing_bit);
    __ stw_PPC(R5_scratch1, in_bytes(JavaThread::popframe_condition_offset()), R24_thread);

    // Empty the expression stack, as in normal exception handling.
    __ empty_expression_stack();
    __ unlock_if_synchronized_method(vtos, /* throw_monitor_exception */ false, /* install_monitor_exception */ false);

    // Check to see whether we are returning to a deoptimized frame.
    // (The PopFrame call ensures that the caller of the popped frame is
    // either interpreted or compiled and deoptimizes it if compiled.)
    // Note that we don't compare the return PC against the
    // deoptimization blob's unpack entry because of the presence of
    // adapter frames in C2.
    Label Lcaller_not_deoptimized;
    Register return_pc = R3_ARG1_PPC;
    __ ld_PPC(return_pc, 0, R1_SP_PPC);
    __ ld_PPC(return_pc, _abi_PPC(lr), return_pc);
    __ call_VM_leaf(CAST_FROM_FN_PTR(address, InterpreterRuntime::interpreter_contains), return_pc);
    __ cmpdi_PPC(CCR0, R3_RET_PPC, 0);
    __ bne_PPC(CCR0, Lcaller_not_deoptimized);

    // The deoptimized case.
    // In this case, we can't call dispatch_next() after the frame is
    // popped, but instead must save the incoming arguments and restore
    // them after deoptimization has occurred.
    __ ld_PPC(R4_ARG2_PPC, in_bytes(Method::const_offset()), R27_method);
    __ lhz_PPC(R4_ARG2_PPC /* number of params */, in_bytes(ConstMethod::size_of_parameters_offset()), R4_ARG2_PPC);
    __ slwi_PPC(R4_ARG2_PPC, R4_ARG2_PPC, Interpreter::logStackElementSize);
    __ addi_PPC(R5_ARG3_PPC, R26_locals, Interpreter::stackElementSize);
    __ subf_PPC(R5_ARG3_PPC, R4_ARG2_PPC, R5_ARG3_PPC);
    // Save these arguments.
    __ call_VM_leaf(CAST_FROM_FN_PTR(address, Deoptimization::popframe_preserve_args), R24_thread, R4_ARG2_PPC, R5_ARG3_PPC);

    // Inform deoptimization that it is responsible for restoring these arguments.
    __ load_const_optimized(R5_scratch1, JavaThread::popframe_force_deopt_reexecution_bit);
    __ stw_PPC(R5_scratch1, in_bytes(JavaThread::popframe_condition_offset()), R24_thread);

    // Return from the current method into the deoptimization blob. Will eventually
    // end up in the deopt interpeter entry, deoptimization prepared everything that
    // we will reexecute the call that called us.
    __ pop_java_frame();
    __ ret();

    // The non-deoptimized case.
    __ bind(Lcaller_not_deoptimized);

    // Clear the popframe condition flag.
    __ li_PPC(R0, 0);
    __ stw_PPC(R0, in_bytes(JavaThread::popframe_condition_offset()), R24_thread);

    // Get out of the current method and re-execute the call that called us.
    // pop_java_frame(false) :
    __ ld(R8_FP, R8_FP, _abi(fp));
    __ sub(R5_scratch1, R21_sender_SP, R2_SP); // size of pop frame
    __ mv(R2_SP, R21_sender_SP);

    __ restore_interpreter_state(R5_scratch1);
    __ ld_PPC(R6_scratch2, _ijava_state(top_frame_sp), R8_FP);
    __ resize_frame_absolute(R6_scratch2, R8_FP);
    if (ProfileInterpreter) {
      __ set_method_data_pointer_for_bcp();
      __ ld_PPC(R5_scratch1, 0, R1_SP_PPC);
      __ std_PPC(R28_mdx_PPC, _ijava_state(mdx), R5_scratch1);
    }
#if INCLUDE_JVMTI
    Label L_done;

    __ lbz_PPC(R5_scratch1, 0, R22_bcp);
    __ cmpwi_PPC(CCR0, R5_scratch1, Bytecodes::_invokestatic);
    __ bne_PPC(CCR0, L_done);

    // The member name argument must be restored if _invokestatic is re-executed after a PopFrame call.
    // Detect such a case in the InterpreterRuntime function and return the member name argument, or NULL.
    __ ld_PPC(R4_ARG2_PPC, 0, R26_locals);
    __ MacroAssembler::call_VM(R4_ARG2_PPC, CAST_FROM_FN_PTR(address, InterpreterRuntime::member_name_arg_or_null), R4_ARG2_PPC, R27_method, R22_bcp, false);
    __ restore_interpreter_state(/*bcp_and_mdx_only*/ true);
    __ cmpdi_PPC(CCR0, R4_ARG2_PPC, 0);
    __ beq_PPC(CCR0, L_done);
    __ std_PPC(R4_ARG2_PPC, wordSize, R23_esp);
    __ bind(L_done);
#endif // INCLUDE_JVMTI
    __ dispatch_next(vtos);
  }
  // end of JVMTI PopFrame support

  // --------------------------------------------------------------------------
  // Remove activation exception entry.
  // This is jumped to if an interpreted method can't handle an exception itself
  // (we come from the throw/rethrow exception entry above). We're going to call
  // into the VM to find the exception handler in the caller, pop the current
  // frame and return the handler we calculated.
  Interpreter::_remove_activation_entry = __ pc();
  {
    __ pop_ptr(Rexception);
    __ verify_thread();
    __ verify_oop(Rexception);
    __ std_PPC(Rexception, in_bytes(JavaThread::vm_result_offset()), R24_thread);

    __ unlock_if_synchronized_method(vtos, /* throw_monitor_exception */ false, true);
    __ notify_method_exit(false, vtos, InterpreterMacroAssembler::SkipNotifyJVMTI, false);

    __ get_vm_result(Rexception);

    // We are done with this activation frame; find out where to go next.
    // The continuation point will be an exception handler, which expects
    // the following registers set up:
    //
    // RET:  exception oop
    // ARG2: Issuing PC (see generate_exception_blob()), only used if the caller is compiled.

    Register return_pc = R31; // Needs to survive the runtime call.
    __ ld_PPC(return_pc, 0, R1_SP_PPC);
    __ ld_PPC(return_pc, _abi_PPC(lr), return_pc);
    __ call_VM_leaf(CAST_FROM_FN_PTR(address, SharedRuntime::exception_handler_for_return_address), R24_thread, return_pc);

    // Remove the current activation.
    __ pop_java_frame(false);

    __ mr_PPC(R4_ARG2_PPC, return_pc);
    __ mtlr_PPC(R3_RET_PPC);
    __ mr_PPC(R3_RET_PPC, Rexception);
    __ blr_PPC();
  }
}

// JVMTI ForceEarlyReturn support.
// Returns "in the middle" of a method with a "fake" return value.
address TemplateInterpreterGenerator::generate_earlyret_entry_for(TosState state) {

  Register Rscratch1 = R5_scratch1,
           Rscratch2 = R6_scratch2;

  address entry = __ pc();
  __ empty_expression_stack();

  __ load_earlyret_value(state, Rscratch1);

  __ ld_PPC(Rscratch1, in_bytes(JavaThread::jvmti_thread_state_offset()), R24_thread);
  // Clear the earlyret state.
  __ li_PPC(R0, 0);
  __ stw_PPC(R0, in_bytes(JvmtiThreadState::earlyret_state_offset()), Rscratch1);

  __ remove_activation(state, false, false);
  // Copied from TemplateTable::_return.
  // Restoration of lr done by remove_activation.
  switch (state) {
    // Narrow result if state is itos but result type is smaller.
    case btos:
    case ztos:
    case ctos:
    case stos:
    case itos: __ narrow(R25_tos); /* fall through */
    case ltos:
    case atos: __ mr_PPC(R3_RET_PPC, R25_tos); break;
    case ftos:
    case dtos: __ fmr_PPC(F1_RET_PPC, F23_ftos); break;
    case vtos: // This might be a constructor. Final fields (and volatile fields on RISCV64) need
               // to get visible before the reference to the object gets stored anywhere.
               __ membar(Assembler::StoreStore); break;
    default  : ShouldNotReachHere();
  }
  __ blr_PPC();

  return entry;
} // end of ForceEarlyReturn support

//-----------------------------------------------------------------------------
// Helper for vtos entry point generation

void TemplateInterpreterGenerator::set_vtos_entry_points(Template* t,
                                                         address& bep,
                                                         address& cep,
                                                         address& sep,
                                                         address& aep,
                                                         address& iep,
                                                         address& lep,
                                                         address& fep,
                                                         address& dep,
                                                         address& vep) {
  assert(t->is_valid() && t->tos_in() == vtos, "illegal template");
  Label L;

  aep = __ pc();  __ push_ptr();  __ j(L);
  fep = __ pc();  __ push_f();    __ j(L);
  dep = __ pc();  __ push_d();    __ j(L);
  lep = __ pc();  __ push_l();    __ j(L);
  __ align(32, 12, 24); // align L
  bep = cep = sep =
  iep = __ pc();  __ push_i();
  vep = __ pc();
  __ bind(L);
  generate_and_dispatch(t);
}

//-----------------------------------------------------------------------------

// Non-product code
#ifndef PRODUCT
address TemplateInterpreterGenerator::generate_trace_code(TosState state) {
  //__ flush_bundle();
  address entry = __ pc();

  const char *bname = NULL;
  uint tsize = 0;
  switch(state) {
  case ftos:
    bname = "trace_code_ftos {";
    tsize = 2;
    break;
  case btos:
    bname = "trace_code_btos {";
    tsize = 2;
    break;
  case ztos:
    bname = "trace_code_ztos {";
    tsize = 2;
    break;
  case ctos:
    bname = "trace_code_ctos {";
    tsize = 2;
    break;
  case stos:
    bname = "trace_code_stos {";
    tsize = 2;
    break;
  case itos:
    bname = "trace_code_itos {";
    tsize = 2;
    break;
  case ltos:
    bname = "trace_code_ltos {";
    tsize = 3;
    break;
  case atos:
    bname = "trace_code_atos {";
    tsize = 2;
    break;
  case vtos:
    // Note: In case of vtos, the topmost of stack value could be a int or doubl
    // In case of a double (2 slots) we won't see the 2nd stack value.
    // Maybe we simply should print the topmost 3 stack slots to cope with the problem.
    bname = "trace_code_vtos {";
    tsize = 2;

    break;
  case dtos:
    bname = "trace_code_dtos {";
    tsize = 3;
    break;
  default:
    ShouldNotReachHere();
  }
  BLOCK_COMMENT(bname);

  // Support short-cut for TraceBytecodesAt.
  // Don't call into the VM if we don't want to trace to speed up things.
  Label Lskip_vm_call;
  if (TraceBytecodesAt > 0 && TraceBytecodesAt < max_intx) {
    __ li(R5_scratch1, (address) &TraceBytecodesAt);
    __ li(R6_scratch2, (address) &BytecodeCounter::_counter_value);
    __ ld(R5_scratch1, R5_scratch1, 0);
    __ lw(R6_scratch2, R6_scratch2, 0);
    __ blt(R6_scratch2, R5_scratch1, Lskip_vm_call);
  }

  __ push(state);
  // Load 2 topmost expression stack values.
  __ ld(R13_ARG3, R23_esp, tsize*Interpreter::stackElementSize);
  __ ld(R12_ARG2, R23_esp, Interpreter::stackElementSize);
  __ sd(R1_RA, R8_FP, _ijava_state(saved_ra));
  __ call_VM(noreg, CAST_FROM_FN_PTR(address, InterpreterRuntime::trace_bytecode), /* unused */ R11_ARG1, R12_ARG2, R13_ARG3, false);
  __ ld(R1_RA, R8_FP, _ijava_state(saved_ra));
  __ pop(state);

  if (TraceBytecodesAt > 0 && TraceBytecodesAt < max_intx) {
    __ bind(Lskip_vm_call);
  }
  __ ret();
  BLOCK_COMMENT("} trace_code");
  return entry;
}

void TemplateInterpreterGenerator::count_bytecode() {
  __ li(R5_scratch1, &BytecodeCounter::_counter_value);
  __ lwu(R6_scratch2, R5_scratch1, 0);
  __ addi(R6_scratch2, R0, 1);
  __ sw(R6_scratch2, R5_scratch1, 0);
}

void TemplateInterpreterGenerator::histogram_bytecode(Template* t) {
  int offs = __ load_const_optimized(R5_scratch1, (address) &BytecodeHistogram::_counters[t->bytecode()], R6_scratch2, true);
  __ lwu(R6_scratch2, R5_scratch1, offs);
  __ addi(R6_scratch2, R6_scratch2, 1);
  __ sw(R6_scratch2, R5_scratch1, offs);
}

void TemplateInterpreterGenerator::histogram_bytecode_pair(Template* t) {
  tty->print_cr("count_bytecode_pair: %p", __ pc());
  const Register addr = R5_scratch1,
                 tmp  = R6_scratch2;
  // Get index, shift out old bytecode, bring in new bytecode, and store it.
  // _index = (_index >> log2_number_of_codes) |
  //          (bytecode << log2_number_of_codes);
  int offs1 = __ load_const_optimized(addr, (address)&BytecodePairHistogram::_index, tmp, true);
  __ lwz_PPC(tmp, offs1, addr);
  __ srwi_PPC(tmp, tmp, BytecodePairHistogram::log2_number_of_codes);
  __ ori_PPC(tmp, tmp, ((int) t->bytecode()) << BytecodePairHistogram::log2_number_of_codes);
  __ stw_PPC(tmp, offs1, addr);

  // Bump bucket contents.
  // _counters[_index] ++;
  int offs2 = __ load_const_optimized(addr, (address)&BytecodePairHistogram::_counters, R0, true);
  __ sldi_PPC(tmp, tmp, LogBytesPerInt);
  __ add_PPC(addr, tmp, addr);
  __ lwz_PPC(tmp, offs2, addr);
  __ addi_PPC(tmp, tmp, 1);
  __ stw_PPC(tmp, offs2, addr);
}

void TemplateInterpreterGenerator::trace_bytecode(Template* t) {
  // Call a little run-time stub to avoid blow-up for each bytecode.
  // The run-time runtime saves the right registers, depending on
  // the tosca in-state for the given template.

  assert(Interpreter::trace_code(t->tos_in()) != NULL,
         "entry must have been generated");

  // Note: we destroy LR here.
  __ li(R5_scratch1, Interpreter::trace_code(t->tos_in()));
  __ jalr (R5_scratch1);
}

void TemplateInterpreterGenerator::stop_interpreter_at() {
  tty->print_cr("stop_interpreter_at: %p", __ pc());
  Label L;
  int offs1 = __ load_const_optimized(R5_scratch1, (address) &StopInterpreterAt, R0, true);
  int offs2 = __ load_const_optimized(R6_scratch2, (address) &BytecodeCounter::_counter_value, R0, true);
  __ ld_PPC(R5_scratch1, offs1, R5_scratch1);
  __ lwa_PPC(R6_scratch2, offs2, R6_scratch2);
  __ cmpd_PPC(CCR0, R6_scratch2, R5_scratch1);
  __ bne_PPC(CCR0, L);
  __ illtrap();
  __ bind(L);
}

#endif // !PRODUCT
