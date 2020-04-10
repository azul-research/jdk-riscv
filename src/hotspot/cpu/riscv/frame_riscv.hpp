/*
 * Copyright (c) 2000, 2019, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, 2015 SAP SE. All rights reserved.
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

#ifndef CPU_RISCV_FRAME_RISCV_HPP
#define CPU_RISCV_FRAME_RISCV_HPP

#include "runtime/synchronizer.hpp"

/* RISC-V C stack frames layout:

      +->          .
      |   +=================+   |
      |   | return address  |   |
      |   |   previous fp ------+
      |   | saved registers |
      |   | local variables |
      |   |       ...       | <-+
      |   +=================+   |
      |   | return address  |   |
      +------ previous fp   |   |
          | saved registers |   |
          | local variables |   |
  $fp --> |       ...       |   |
          +=================+   |
          | return address  |   |
          |   previous fp ------+
          | saved registers |
  $sp --> | local variables |
          +-----------------+
              stack grows
                  down
                   |
                   V


  Java entry frame layout:

   fp --> |       ...       |   |
          +=================+   |
          | return address  |   |   }
          |   previous fp ------+   }  frame_abi
          +-----------------+
          |                 |       }
          | saved registers |       }  spill_nonvolatiles
          |                 |       }
          +-----------------+
          |                 |       }
          |  frame locals   |       }  entry_frame_locals
          |                 |       }
          +-----------------+
          |   argument 0    |
          |       ...       |
   sp --> |   argument n    |
          +=================+ <-- esp (it's equals to SP or SP + 8, depends on )
                   |
                   V


  Parent Java frame layout:

          |       ...       |   |
          +=================+   |
          | return address  |   |   }
          |   previous fp ------+   }  frame_abi
          +-----------------+
          |                 |       }
          |   ijava state   |       }  ijava_state
          |                 |       }
          +-----------------+
          |                 |       }
          |    monitors     |       }  (optional)
          |                 |       }
          +-----------------+
          |                 |
          |   used part of  |
          |  operand stack  |
          |                 |
          +-----------------+
locals--> |   argument 0    |       }
          |       ...       |       }
          |   argument n    |       }  arguments and locals of
          +-----------------+       }     the next method
          |                 |       }
          |      locals     |       }
          |                 |       }
          +=================+
                   |
                   V


  Top Java frame layout:

locals--> |   argument 0    |
          |       ...       |
          |   argument n    |
          |-----------------|
          |                 |
          |     locals      |
  fp -->  |                 |   |
          +=================+   |
          | return address  |   |   }
          |   previous fp ------+   }  frame_abi
          +-----------------+
          |                 |       }
          |   ijava state   |       }  ijava_state
          |                 |       }
          +-----------------+
          |                 |       }
          |    monitors     |       }  (optional)
monitor-> |                 |       }
          +-----------------+
          |                 |
          |   used part of  |
          |  operand stack  |
          |                 |
          +-----------------+
  esp --> |       ___       |
          |                 |
          |                 |
   sp --> +=================+
                   |
                   V

*/

 public:

  static const int alignment_in_bytes = 16;

  struct abi_frame {
      uint64_t fp;
      uint64_t ra; //_16
  };

  struct spill_nonvolatiles {
    double f27;
    double f26;                                   //_16
    double f25;
    double f24;                                   //_16
    double f23;
    double f22;                                   //_16
    double f21;
    double f20;                                   //_16
    double f19;
    double f18;                                   //_16
    double f9;
    double f8;                                    //_16

    uint64_t r27;
    uint64_t r26;                                 //_16
    uint64_t r25;
    uint64_t r24;                                 //_16
    uint64_t r23;
    uint64_t r22;                                 //_16
    uint64_t r21;
    uint64_t r20;                                 //_16
    uint64_t r19;
    uint64_t r18;                                 //_16
    uint64_t r9;
    uint64_t r2;                                  //_16
  };

  struct entry_frame_locals {
    uint64_t call_wrapper_address;
    uint64_t result_address;                      //_16
    uint64_t result_type;
    uint64_t arguments_tos_address;               //_16
  };

  enum {
    abi_frame_size = sizeof(abi_frame),
    spill_nonvolatiles_size = sizeof(spill_nonvolatiles),
    entry_frame_locals_size = sizeof(entry_frame_locals),
    entry_frame_size = abi_frame_size + spill_nonvolatiles_size + entry_frame_locals_size,
  };

  #define _abi(_component) \
         (int) (-frame::abi_frame_size + offset_of(frame::abi_frame, _component))

  #define _entry_frame_locals(_component) \
        (int) (-frame::entry_frame_size + offset_of(frame::entry_frame_locals, _component))

  struct ijava_state {
#ifdef ASSERT
    uint64_t ijava_reserved; // Used for assertion.
#endif
    uint64_t method;
    uint64_t mirror;
    uint64_t locals;
    uint64_t monitors;
    uint64_t cpoolCache;
    uint64_t bcp;
    uint64_t esp;
    uint64_t mdx;
    uint64_t top_frame_sp; // Maybe define parent_frame_abi and move there.
    uint64_t sender_sp;
    // Slots only needed for native calls. Maybe better to move elsewhere.
    uint64_t oop_tmp;
    uint64_t lresult;
    uint64_t fresult;
  };

  enum {
    ijava_state_size = sizeof(ijava_state),
    frame_header_size = abi_frame_size + ijava_state_size
  };


#define _ijava_state(_component) \
        (int) (-frame::frame_header_size + offset_of(frame::ijava_state, _component))




// PPC STRUCTURES:

  struct abi_minframe_ppc { // FIXME_RISCV this structure must be removed
    uint64_t callers_sp;
    uint64_t cr;                                  //_16
    uint64_t lr;
    uint64_t toc;                                 //_16
    // nothing to add here!
    // aligned to frame::alignment_in_bytes (16)
  };

  enum { // FIXME_RISCV this enum must be removed
    abi_minframe_ppc_size = sizeof(abi_minframe_ppc)
  };


  struct abi_reg_args_ppc : abi_minframe_ppc { // FIXME_RISCV this structure must be removed
    uint64_t carg_1;
    uint64_t carg_2;                              //_16
    uint64_t carg_3;
    uint64_t carg_4;                              //_16
    uint64_t carg_5;
    uint64_t carg_6;                              //_16
    uint64_t carg_7;
    uint64_t carg_8;                              //_16
    // aligned to frame::alignment_in_bytes (16)
  };

  enum { // FIXME_RISCV this enum must be removed
    abi_reg_args_ppc_size = sizeof(abi_reg_args_ppc)
  };


  struct abi_reg_args_spill_ppc : abi_reg_args_ppc { // FIXME_RISCV this structure must be removed
    // additional spill slots
    uint64_t spill_ret;
    uint64_t spill_fret;                          //_16
    // aligned to frame::alignment_in_bytes (16)
  };

  enum { // FIXME_RISCV this enum must be removed
    abi_reg_args_spill_ppc_size = sizeof(abi_reg_args_spill_ppc)
  };

  // FIXME_RISCV this macro must be removed
  #define _abi_reg_args_spill_ppc(_component) \
          (offset_of(frame::abi_reg_args_spill_ppc, _component))


  // FIXME_RISCV this define must be removed
  #define _abi_PPC(_component) \
          (offset_of(frame::abi_reg_args_ppc, _component))

  // Frame layout for the Java template interpreter on PPC64.
  //
  // In these figures the stack grows upwards, while memory grows
  // downwards. Square brackets denote regions possibly larger than
  // single 64 bit slots.

  struct parent_ijava_frame_abi : abi_minframe_ppc {};
  struct top_ijava_frame_abi : abi_minframe_ppc {};

  enum {
    parent_ijava_frame_abi_size = sizeof(parent_ijava_frame_abi)
  };

  //  Frame layout for JIT generated methods
  //
  //  In these figures the stack grows upwards, while memory grows
  //  downwards. Square brackets denote regions possibly larger than single
  //  64 bit slots.
  //
  //  STACK (interpreted Java calls JIT generated Java):
  //          [JIT_FRAME]                                <-- SP (mod 16 = 0)
  //          [TOP_IJAVA_FRAME]
  //         ...
  //
  //  JIT_FRAME (is a C frame according to RISCV-64 ABI):
  //          [out_preserve]
  //          [out_args]
  //          [spills]
  //          [pad_1]
  //          [monitor] (optional)
  //       ...
  //          [monitor] (optional)
  //          [pad_2]
  //          [in_preserve] added / removed by prolog / epilog
  //

  // JIT_ABI (TOP and PARENT)

  struct jit_abi {
    uint64_t callers_sp;
    uint64_t cr;
    uint64_t lr;
    uint64_t toc;
    // Nothing to add here!
    // NOT ALIGNED to frame::alignment_in_bytes (16).
  };

  struct jit_out_preserve : jit_abi {
    // Nothing to add here!
  };

  struct jit_in_preserve {
    // Nothing to add here!
  };

  enum {
    jit_out_preserve_size = sizeof(jit_out_preserve),
    jit_in_preserve_size  = sizeof(jit_in_preserve)
  };

  struct jit_monitor {
    uint64_t monitor[1];
  };

  enum {
    jit_monitor_size = sizeof(jit_monitor),
  };

 private:

  //  STACK:
  //            ...
  //            [THIS_FRAME]             <-- this._sp (stack pointer for this frame)
  //            [CALLER_FRAME]           <-- this.fp() (_sp of caller's frame)
  //            ...
  //

  // The frame's stack pointer before it has been extended by a c2i adapter;
  // needed by deoptimization
  intptr_t* _unextended_sp;

  // frame pointer for this frame
  intptr_t* _fp;

 public:

  // Accessors for fields
  intptr_t* fp() const { return _fp; }

  // Accessors for ABIs
  inline abi_frame* own_abi()     const { return (abi_frame*) (_fp - abi_frame_size); }
  inline abi_frame* callers_abi() const { return (abi_frame*) (own_abi()->fp - abi_frame_size); }

 private:

  // Find codeblob and set deopt_state.
  inline void find_codeblob_and_set_pc_and_deopt_state(address pc);

 public:

  // Constructors
  inline frame(intptr_t* sp, intptr_t* fp);
  inline frame(intptr_t* sp, intptr_t* fp, address pc);
  inline frame(intptr_t* sp, intptr_t* fp, address pc, intptr_t* unextended_sp);

 private:

  intptr_t* compiled_sender_sp(CodeBlob* cb) const;
  address*  compiled_sender_pc_addr(CodeBlob* cb) const;
  address*  sender_pc_addr(void) const;

 public:

  inline ijava_state* get_ijava_state() const;
  // Some convenient register frame setters/getters for deoptimization.
  inline intptr_t* interpreter_frame_esp() const;
  inline void interpreter_frame_set_cpcache(ConstantPoolCache* cp);
  inline void interpreter_frame_set_esp(intptr_t* esp);
  inline void interpreter_frame_set_top_frame_sp(intptr_t* top_frame_sp);
  inline void interpreter_frame_set_sender_sp(intptr_t* sender_sp);

  // Size of a monitor in bytes.
  static int interpreter_frame_monitor_size_in_bytes();

  // The size of a cInterpreter object.
  static inline int interpreter_frame_cinterpreterstate_size_in_bytes();

 private:

  ConstantPoolCache** interpreter_frame_cpoolcache_addr() const;

 public:

  // Additional interface for entry frames:
  inline entry_frame_locals* get_entry_frame_locals() const {
    return (entry_frame_locals*) (((address) fp()) - entry_frame_locals_size);
  }

  inline intptr_t* sender_fp() const { return (intptr_t*)own_abi()->fp; }

  enum {
    // normal return address is 1 bundle past PC
    pc_return_offset = 0
  };

  static jint interpreter_frame_expression_stack_direction() { return -1; }

#endif // CPU_RISCV_FRAME_RISCV_HPP
