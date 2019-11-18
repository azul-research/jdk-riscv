/*
 * Copyright (c) 1997, 2019, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, 2019 SAP SE. All rights reserved.
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

#ifndef CPU_RISCV_VM_VERSION_RISCV_HPP
#define CPU_RISCV_VM_VERSION_RISCV_HPP

#include "runtime/globals_extension.hpp"
#include "runtime/vm_version.hpp"

class VM_Version: public Abstract_VM_Version {
protected:
  enum Feature_Flag {
    num_features // last entry to count features
  };
  enum Feature_Flag_Set {
    unknown_m             = 0,
    all_features_m        = (unsigned long)-1
  };

  static bool _is_determine_features_test_running;

  static void print_features();
  static void determine_features(); // also measures cache line size
public:
  // Initialization
  static void initialize();
  static void check_virtualizations();

  // Override Abstract_VM_Version implementation
  static void print_platform_virtualization_info(outputStream*);

  // Override Abstract_VM_Version implementation
  static bool use_biased_locking();

  // FIXME_RISCV: check whether RISCV supports fast class initialization checks for static methods.
  static bool supports_fast_class_init_checks() { return false; }

  static bool is_determine_features_test_running() { return _is_determine_features_test_running; }

    // CPU instruction support
    static bool has_fsqrt()   { return false; }
    static bool has_fsqrts()  { return false; }
    static bool has_isel()    { return false; }
    static bool has_lxarxeh() { return false; }
    static bool has_cmpb()    { return false; }
    static bool has_popcntb() { return false; }
    static bool has_popcntw() { return false; }
    static bool has_fcfids()  { return false; }
    static bool has_vand()    { return false; }
    static bool has_lqarx()   { return false; }
    static bool has_vcipher() { return false; }
    static bool has_vpmsumb() { return false; }
    static bool has_mfdscr()  { return false; }
    static bool has_vsx()     { return false; }
    static bool has_ldbrx()   { return false; }
    static bool has_stdbrx()  { return false; }
    static bool has_vshasig() { return false; }
    static bool has_tm()      { return false; }
    static bool has_darn()    { return false; }

    static bool has_mtfprd()  { return false; } // alias for P8

    // Assembler testing
  static void allow_all();
  static void revert();

  // POWER 8: DSCR current value.
  static uint64_t _dscr_val;
};

#endif // CPU_RISCV_VM_VERSION_RISCV_HPP
