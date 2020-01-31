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

#include "precompiled.hpp"
#include "jvm.h"
#include "asm/assembler.inline.hpp"
#include "asm/macroAssembler.inline.hpp"
#include "compiler/disassembler.hpp"
#include "memory/resourceArea.hpp"
#include "runtime/java.hpp"
#include "runtime/os.hpp"
#include "runtime/stubCodeGenerator.hpp"
#include "utilities/align.hpp"
#include "utilities/defaultStream.hpp"
#include "utilities/globalDefinitions.hpp"
#include "vm_version_riscv.hpp"

#include <sys/sysinfo.h>
#if defined(_AIX)
#include <libperfstat.h>
#endif

#if defined(LINUX) && defined(VM_LITTLE_ENDIAN)
#include <sys/auxv.h>

#ifndef RISCV_FEATURE2_HTM_NOSC
#define RISCV_FEATURE2_HTM_NOSC (1 << 24)
#endif
#endif

bool VM_Version::_is_determine_features_test_running = false;
uint64_t VM_Version::_dscr_val = 0;

// FIXME_RISCV begin
#define MSG(flag)   ;/* /
  if (flag && !FLAG_IS_DEFAULT(flag))                                  \
      jio_fprintf(defaultStream::error_stream(),                       \
                  "warning: -XX:+" #flag " requires -XX:+UseSIGTRAP\n" \
                  "         -XX:+" #flag " will be disabled!\n");
*/// FIXME_RISCV end
void VM_Version::initialize() {

  // Test which instructions are supported and measure cache line size.
  determine_features();

  // If PowerArchitectureRISCV64 hasn't been specified explicitly determine from features.
  /* // FIXME_RISCV begin
  if (FLAG_IS_DEFAULT(PowerArchitectureRISCV64)) {
    if (VM_Version::has_darn()) {
      FLAG_SET_ERGO(PowerArchitectureRISCV64, 9);
    } else if (VM_Version::has_lqarx()) {
      FLAG_SET_ERGO(PowerArchitectureRISCV64, 8);
    } else if (VM_Version::has_popcntw()) {
      FLAG_SET_ERGO(PowerArchitectureRISCV64, 7);
    } else if (VM_Version::has_cmpb()) {
      FLAG_SET_ERGO(PowerArchitectureRISCV64, 6);
    } else if (VM_Version::has_popcntb()) {
      FLAG_SET_ERGO(PowerArchitectureRISCV64, 5);
    } else {
      FLAG_SET_ERGO(PowerArchitectureRISCV64, 0);
    }
  }

  bool PowerArchitectureRISCV64_ok = false;
  switch (PowerArchitectureRISCV64) {
    case 9: if (!VM_Version::has_darn()   ) break;
    case 8: if (!VM_Version::has_lqarx()  ) break;
    case 7: if (!VM_Version::has_popcntw()) break;
    case 6: if (!VM_Version::has_cmpb()   ) break;
    case 5: if (!VM_Version::has_popcntb()) break;
    case 0: PowerArchitectureRISCV64_ok = true; break;
    default: break;
  }
  guarantee(PowerArchitectureRISCV64_ok, "PowerArchitectureRISCV64 cannot be set to "
            UINTX_FORMAT " on this machine", PowerArchitectureRISCV64);

  if (!UseSIGTRAP) {
    MSG(TrapBasedICMissChecks);
    MSG(TrapBasedNotEntrantChecks);
    MSG(TrapBasedNullChecks);
    FLAG_SET_ERGO(TrapBasedNotEntrantChecks, false);
    FLAG_SET_ERGO(TrapBasedNullChecks,       false);
    FLAG_SET_ERGO(TrapBasedICMissChecks,     false);
  }
  */// FIXME_RISCV end

#ifdef COMPILER2
  if (!UseSIGTRAP) {
    MSG(TrapBasedRangeChecks);
    FLAG_SET_ERGO(TrapBasedRangeChecks, false);
  }

  // On Power6 test for section size.
  if (PowerArchitectureRISCV64 == 6) {
    determine_section_size();
  // TODO: RISCV port } else {
  // TODO: RISCV port PdScheduling::power6SectorSize = 0x20;
  }

  if (PowerArchitectureRISCV64 >= 8) {
    if (FLAG_IS_DEFAULT(SuperwordUseVSX)) {
      FLAG_SET_ERGO(SuperwordUseVSX, true);
    }
  } else {
    if (SuperwordUseVSX) {
      warning("SuperwordUseVSX specified, but needs at least Power8.");
      FLAG_SET_DEFAULT(SuperwordUseVSX, false);
    }
  }
  MaxVectorSize = SuperwordUseVSX ? 16 : 8;

  if (PowerArchitectureRISCV64 >= 9) {
    if (FLAG_IS_DEFAULT(UseCountTrailingZerosInstructionsRISCV64)) {
      FLAG_SET_ERGO(UseCountTrailingZerosInstructionsRISCV64, true);
    }
    if (FLAG_IS_DEFAULT(UseCharacterCompareIntrinsics)) {
      FLAG_SET_ERGO(UseCharacterCompareIntrinsics, true);
    }
  } else {
    if (UseCountTrailingZerosInstructionsRISCV64) {
      warning("UseCountTrailingZerosInstructionsRISCV64 specified, but needs at least Power9.");
      FLAG_SET_DEFAULT(UseCountTrailingZerosInstructionsRISCV64, false);
    }
    if (UseCharacterCompareIntrinsics) {
      warning("UseCharacterCompareIntrinsics specified, but needs at least Power9.");
      FLAG_SET_DEFAULT(UseCharacterCompareIntrinsics, false);
    }
  }
#endif

  // Create and print feature-string.
/* // FIXME_RISCV begin
  char buf[(num_features+1) * 16]; // Max 16 chars per feature.
  jio_snprintf(buf, sizeof(buf),
               "riscv64%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
               (has_fsqrt()   ? " fsqrt"   : ""),
               (has_isel()    ? " isel"    : ""),
               (has_lxarxeh() ? " lxarxeh" : ""),
               (has_cmpb()    ? " cmpb"    : ""),
               (has_popcntb() ? " popcntb" : ""),
               (has_popcntw() ? " popcntw" : ""),
               (has_fcfids()  ? " fcfids"  : ""),
               (has_vand()    ? " vand"    : ""),
               (has_lqarx()   ? " lqarx"   : ""),
               (has_vcipher() ? " aes"     : ""),
               (has_vpmsumb() ? " vpmsumb" : ""),
               (has_mfdscr()  ? " mfdscr"  : ""),
               (has_vsx()     ? " vsx"     : ""),
               (has_ldbrx()   ? " ldbrx"   : ""),
               (has_stdbrx()  ? " stdbrx"  : ""),
               (has_vshasig() ? " sha"     : ""),
               (has_tm()      ? " rtm"     : ""),
               (has_darn()    ? " darn"    : "")
               // Make sure number of %s matches num_features!
              );
  _features_string = os::strdup(buf);
  if (Verbose) {
    print_features();
  }

  // RISCV64 supports 8-byte compare-exchange operations (see Atomic::cmpxchg)
  // and 'atomic long memory ops' (see Unsafe_GetLongVolatile).
  _supports_cx8 = true;

  // Used by C1.
  _supports_atomic_getset4 = true;
  _supports_atomic_getadd4 = true;
  _supports_atomic_getset8 = true;
  _supports_atomic_getadd8 = true;

  UseSSE = 0; // Only on x86 and x64
*/ // FIXME_RISCV end
  intx cache_line_size = L1_data_cache_line_size();

  if (FLAG_IS_DEFAULT(AllocatePrefetchStyle)) AllocatePrefetchStyle = 1;

  if (AllocatePrefetchStyle == 4) {
    AllocatePrefetchStepSize = cache_line_size; // Need exact value.
    if (FLAG_IS_DEFAULT(AllocatePrefetchLines)) AllocatePrefetchLines = 12; // Use larger blocks by default.
    if (AllocatePrefetchDistance < 0) AllocatePrefetchDistance = 2*cache_line_size; // Default is not defined?
  } else {
    if (cache_line_size > AllocatePrefetchStepSize) AllocatePrefetchStepSize = cache_line_size;
    if (FLAG_IS_DEFAULT(AllocatePrefetchLines)) AllocatePrefetchLines = 3; // Optimistic value.
    if (AllocatePrefetchDistance < 0) AllocatePrefetchDistance = 3*cache_line_size; // Default is not defined?
  }

  assert(AllocatePrefetchLines > 0, "invalid value");
  if (AllocatePrefetchLines < 1) { // Set valid value in product VM.
    AllocatePrefetchLines = 1; // Conservative value.
  }

  if (AllocatePrefetchStyle == 3 && AllocatePrefetchDistance < cache_line_size) {
    AllocatePrefetchStyle = 1; // Fall back if inappropriate.
  }

  assert(AllocatePrefetchStyle >= 0, "AllocatePrefetchStyle should be positive");

  /* // FIXME_RISCV begin
  // If running on Power8 or newer hardware, the implementation uses the available vector instructions.
  // In all other cases, the implementation uses only generally available instructions.
  if (!UseCRC32Intrinsics) {
    if (FLAG_IS_DEFAULT(UseCRC32Intrinsics)) {
      FLAG_SET_DEFAULT(UseCRC32Intrinsics, true);
    }
  }

  // Implementation does not use any of the vector instructions available with Power8.
  // Their exploitation is still pending (aka "work in progress").
  if (!UseCRC32CIntrinsics) {
    if (FLAG_IS_DEFAULT(UseCRC32CIntrinsics)) {
      FLAG_SET_DEFAULT(UseCRC32CIntrinsics, true);
    }
  }

  // TODO: Provide implementation.
  if (UseAdler32Intrinsics) {
    warning("Adler32Intrinsics not available on this CPU.");
    FLAG_SET_DEFAULT(UseAdler32Intrinsics, false);
  }

  // The AES intrinsic stubs require AES instruction support.
  if (has_vcipher()) {
    if (FLAG_IS_DEFAULT(UseAES)) {
      UseAES = true;
    }
  } else if (UseAES) {
    if (!FLAG_IS_DEFAULT(UseAES))
      warning("AES instructions are not available on this CPU");
    FLAG_SET_DEFAULT(UseAES, false);
  }

  if (UseAES && has_vcipher()) {
    if (FLAG_IS_DEFAULT(UseAESIntrinsics)) {
      UseAESIntrinsics = true;
    }
  } else if (UseAESIntrinsics) {
    if (!FLAG_IS_DEFAULT(UseAESIntrinsics))
      warning("AES intrinsics are not available on this CPU");
    FLAG_SET_DEFAULT(UseAESIntrinsics, false);
  }

  if (UseAESCTRIntrinsics) {
    warning("AES/CTR intrinsics are not available on this CPU");
    FLAG_SET_DEFAULT(UseAESCTRIntrinsics, false);
  }

  if (UseGHASHIntrinsics) {
    warning("GHASH intrinsics are not available on this CPU");
    FLAG_SET_DEFAULT(UseGHASHIntrinsics, false);
  }

  if (FLAG_IS_DEFAULT(UseFMA)) {
    FLAG_SET_DEFAULT(UseFMA, true);
  }

  if (has_vshasig()) {
    if (FLAG_IS_DEFAULT(UseSHA)) {
      UseSHA = true;
    }
  } else if (UseSHA) {
    if (!FLAG_IS_DEFAULT(UseSHA))
      warning("SHA instructions are not available on this CPU");
    FLAG_SET_DEFAULT(UseSHA, false);
  }

  if (UseSHA1Intrinsics) {
    warning("Intrinsics for SHA-1 crypto hash functions not available on this CPU.");
    FLAG_SET_DEFAULT(UseSHA1Intrinsics, false);
  }

  if (UseSHA && has_vshasig()) {
    if (FLAG_IS_DEFAULT(UseSHA256Intrinsics)) {
      FLAG_SET_DEFAULT(UseSHA256Intrinsics, true);
    }
  } else if (UseSHA256Intrinsics) {
    warning("Intrinsics for SHA-224 and SHA-256 crypto hash functions not available on this CPU.");
    FLAG_SET_DEFAULT(UseSHA256Intrinsics, false);
  }

  if (UseSHA && has_vshasig()) {
    if (FLAG_IS_DEFAULT(UseSHA512Intrinsics)) {
      FLAG_SET_DEFAULT(UseSHA512Intrinsics, true);
    }
  } else if (UseSHA512Intrinsics) {
    warning("Intrinsics for SHA-384 and SHA-512 crypto hash functions not available on this CPU.");
    FLAG_SET_DEFAULT(UseSHA512Intrinsics, false);
  }

  if (!(UseSHA1Intrinsics || UseSHA256Intrinsics || UseSHA512Intrinsics)) {
    FLAG_SET_DEFAULT(UseSHA, false);
  }

  if (FLAG_IS_DEFAULT(UseSquareToLenIntrinsic)) {
    UseSquareToLenIntrinsic = true;
  }
  if (FLAG_IS_DEFAULT(UseMulAddIntrinsic)) {
    UseMulAddIntrinsic = true;
  }
  if (FLAG_IS_DEFAULT(UseMultiplyToLenIntrinsic)) {
    UseMultiplyToLenIntrinsic = true;
  }
  if (FLAG_IS_DEFAULT(UseMontgomeryMultiplyIntrinsic)) {
    UseMontgomeryMultiplyIntrinsic = true;
  }
  if (FLAG_IS_DEFAULT(UseMontgomerySquareIntrinsic)) {
    UseMontgomerySquareIntrinsic = true;
  }

  if (UseVectorizedMismatchIntrinsic) {
    warning("UseVectorizedMismatchIntrinsic specified, but not available on this CPU.");
    FLAG_SET_DEFAULT(UseVectorizedMismatchIntrinsic, false);
  }


  // Adjust RTM (Restricted Transactional Memory) flags.
  if (UseRTMLocking) {
    // If CPU or OS do not support TM:
    // Can't continue because UseRTMLocking affects UseBiasedLocking flag
    // setting during arguments processing. See use_biased_locking().
    // VM_Version_init() is executed after UseBiasedLocking is used
    // in Thread::allocate().
    if (PowerArchitectureRISCV64 < 8) {
      vm_exit_during_initialization("RTM instructions are not available on this CPU.");
    }

    if (!has_tm()) {
      vm_exit_during_initialization("RTM is not supported on this OS version.");
    }
  }

  if (UseRTMLocking) {
#if INCLUDE_RTM_OPT
    if (!FLAG_IS_CMDLINE(UseRTMLocking)) {
      // RTM locking should be used only for applications with
      // high lock contention. For now we do not use it by default.
      vm_exit_during_initialization("UseRTMLocking flag should be only set on command line");
    }
#else
    // Only C2 does RTM locking optimization.
    // Can't continue because UseRTMLocking affects UseBiasedLocking flag
    // setting during arguments processing. See use_biased_locking().
    vm_exit_during_initialization("RTM locking optimization is not supported in this VM");
#endif
  } else { // !UseRTMLocking
    if (UseRTMForStackLocks) {
      if (!FLAG_IS_DEFAULT(UseRTMForStackLocks)) {
        warning("UseRTMForStackLocks flag should be off when UseRTMLocking flag is off");
      }
      FLAG_SET_DEFAULT(UseRTMForStackLocks, false);
    }
    if (UseRTMDeopt) {
      FLAG_SET_DEFAULT(UseRTMDeopt, false);
    }
    if (PrintPreciseRTMLockingStatistics) {
      FLAG_SET_DEFAULT(PrintPreciseRTMLockingStatistics, false);
    }
  }

  // This machine allows unaligned memory accesses
  if (FLAG_IS_DEFAULT(UseUnalignedAccesses)) {
    FLAG_SET_DEFAULT(UseUnalignedAccesses, true);
  }

  check_virtualizations();
*/ // FIXME_RISCV end
}

void VM_Version::check_virtualizations() {
#if defined(_AIX)
  int rc = 0;
  perfstat_partition_total_t pinfo;
  rc = perfstat_partition_total(NULL, &pinfo, sizeof(perfstat_partition_total_t), 1);
  if (rc == 1) {
    Abstract_VM_Version::_detected_virtualization = PowerVM;
  }
#else
  const char* info_file = "/proc/riscv64/lparcfg";
  // system_type=...qemu indicates PowerKVM
  // e.g. system_type=IBM pSeries (emulated by qemu)
  char line[500];
  FILE* fp = fopen(info_file, "r");
  if (fp == NULL) {
    return;
  }
  const char* system_type="system_type=";  // in case this line contains qemu, it is KVM
  const char* num_lpars="NumLpars="; // in case of non-KVM : if this line is found it is PowerVM
  bool num_lpars_found = false;

  while (fgets(line, sizeof(line), fp) != NULL) {
    if (strncmp(line, system_type, strlen(system_type)) == 0) {
      if (strstr(line, "qemu") != 0) {
        Abstract_VM_Version::_detected_virtualization = PowerKVM;
        fclose(fp);
        return;
      }
    }
    if (strncmp(line, num_lpars, strlen(num_lpars)) == 0) {
      num_lpars_found = true;
    }
  }
  if (num_lpars_found) {
    Abstract_VM_Version::_detected_virtualization = PowerVM;
  } else {
    Abstract_VM_Version::_detected_virtualization = PowerFullPartitionMode;
  }
  fclose(fp);
#endif
}

void VM_Version::print_platform_virtualization_info(outputStream* st) {
#if defined(_AIX)
  // more info about perfstat API see
  // https://www.ibm.com/support/knowledgecenter/en/ssw_aix_72/com.ibm.aix.prftools/idprftools_perfstat_glob_partition.htm
  int rc = 0;
  perfstat_partition_total_t pinfo;
  memset(&pinfo, 0, sizeof(perfstat_partition_total_t));
  rc = perfstat_partition_total(NULL, &pinfo, sizeof(perfstat_partition_total_t), 1);
  if (rc != 1) {
    return;
  } else {
    st->print_cr("Virtualization type   : PowerVM");
  }
  // CPU information
  perfstat_cpu_total_t cpuinfo;
  memset(&cpuinfo, 0, sizeof(perfstat_cpu_total_t));
  rc = perfstat_cpu_total(NULL, &cpuinfo, sizeof(perfstat_cpu_total_t), 1);
  if (rc != 1) {
    return;
  }

  st->print_cr("Processor description : %s", cpuinfo.description);
  st->print_cr("Processor speed       : %llu Hz", cpuinfo.processorHZ);

  st->print_cr("LPAR partition name           : %s", pinfo.name);
  st->print_cr("LPAR partition number         : %u", pinfo.lpar_id);
  st->print_cr("LPAR partition type           : %s", pinfo.type.b.shared_enabled ? "shared" : "dedicated");
  st->print_cr("LPAR mode                     : %s", pinfo.type.b.donate_enabled ? "donating" : pinfo.type.b.capped ? "capped" : "uncapped");
  st->print_cr("LPAR partition group ID       : %u", pinfo.group_id);
  st->print_cr("LPAR shared pool ID           : %u", pinfo.pool_id);

  st->print_cr("AMS (active memory sharing)   : %s", pinfo.type.b.ams_capable ? "capable" : "not capable");
  st->print_cr("AMS (active memory sharing)   : %s", pinfo.type.b.ams_enabled ? "on" : "off");
  st->print_cr("AME (active memory expansion) : %s", pinfo.type.b.ame_enabled ? "on" : "off");

  if (pinfo.type.b.ame_enabled) {
    st->print_cr("AME true memory in bytes      : %llu", pinfo.true_memory);
    st->print_cr("AME expanded memory in bytes  : %llu", pinfo.expanded_memory);
  }

  st->print_cr("SMT : %s", pinfo.type.b.smt_capable ? "capable" : "not capable");
  st->print_cr("SMT : %s", pinfo.type.b.smt_enabled ? "on" : "off");
  int ocpus = pinfo.online_cpus > 0 ?  pinfo.online_cpus : 1;
  st->print_cr("LPAR threads              : %d", cpuinfo.ncpus/ocpus);
  st->print_cr("LPAR online virtual cpus  : %d", pinfo.online_cpus);
  st->print_cr("LPAR logical cpus         : %d", cpuinfo.ncpus);
  st->print_cr("LPAR maximum virtual cpus : %u", pinfo.max_cpus);
  st->print_cr("LPAR minimum virtual cpus : %u", pinfo.min_cpus);
  st->print_cr("LPAR entitled capacity    : %4.2f", (double) (pinfo.entitled_proc_capacity/100.0));
  st->print_cr("LPAR online memory        : %llu MB", pinfo.online_memory);
  st->print_cr("LPAR maximum memory       : %llu MB", pinfo.max_memory);
  st->print_cr("LPAR minimum memory       : %llu MB", pinfo.min_memory);
#else
  const char* info_file = "/proc/riscv64/lparcfg";
  const char* kw[] = { "system_type=", // qemu indicates PowerKVM
                       "partition_entitled_capacity=", // entitled processor capacity percentage
                       "partition_max_entitled_capacity=",
                       "capacity_weight=", // partition CPU weight
                       "partition_active_processors=",
                       "partition_potential_processors=",
                       "entitled_proc_capacity_available=",
                       "capped=", // 0 - uncapped, 1 - vcpus capped at entitled processor capacity percentage
                       "shared_processor_mode=", // (non)dedicated partition
                       "system_potential_processors=",
                       "pool=", // CPU-pool number
                       "pool_capacity=",
                       "NumLpars=", // on non-KVM machines, NumLpars is not found for full partition mode machines
                       NULL };
  if (!print_matching_lines_from_file(info_file, st, kw)) {
    st->print_cr("  <%s Not Available>", info_file);
  }
#endif
}

bool VM_Version::use_biased_locking() {
#if INCLUDE_RTM_OPT
  // RTM locking is most useful when there is high lock contention and
  // low data contention. With high lock contention the lock is usually
  // inflated and biased locking is not suitable for that case.
  // RTM locking code requires that biased locking is off.
  // Note: we can't switch off UseBiasedLocking in get_processor_features()
  // because it is used by Thread::allocate() which is called before
  // VM_Version::initialize().
  if (UseRTMLocking && UseBiasedLocking) {
    if (FLAG_IS_DEFAULT(UseBiasedLocking)) {
      FLAG_SET_DEFAULT(UseBiasedLocking, false);
    } else {
      warning("Biased locking is not supported with RTM locking; ignoring UseBiasedLocking flag." );
      UseBiasedLocking = false;
    }
  }
#endif
  return UseBiasedLocking;
}

void VM_Version::print_features() {
  tty->print_cr("Version: %s L1_data_cache_line_size=%d", features_string(), L1_data_cache_line_size());
}

#ifdef COMPILER2
// Determine section size on power6: If section size is 8 instructions,
// there should be a difference between the two testloops of ~15 %. If
// no difference is detected the section is assumed to be 32 instructions.
void VM_Version::determine_section_size() {

  int unroll = 80;

  const int code_size = (2* unroll * 32 + 100)*BytesPerInstWord;

  // Allocate space for the code.
  ResourceMark rm;
  CodeBuffer cb("detect_section_size", code_size, 0);
  MacroAssembler* a = new MacroAssembler(&cb);

  uint32_t *code = (uint32_t *)a->pc();
  // Emit code.
  void (*test1)() = (void(*)())(void *)a->pc();

  Label l1;

  a->li(R4, 1);
  a->sldi(R4, R4, 28);
  a->b(l1);
  a->align(CodeEntryAlignment);

  a->bind(l1);

  for (int i = 0; i < unroll; i++) {
    // Schleife 1
    // ------- sector 0 ------------
    // ;; 0
    a->nop();                   // 1
    a->fpnop0();                // 2
    a->fpnop1();                // 3
    a->addi(R4,R4, -1); // 4

    // ;;  1
    a->nop();                   // 5
    a->fmr(F6, F6);             // 6
    a->fmr(F7, F7);             // 7
    a->endgroup();              // 8
    // ------- sector 8 ------------

    // ;;  2
    a->nop();                   // 9
    a->nop();                   // 10
    a->fmr(F8, F8);             // 11
    a->fmr(F9, F9);             // 12

    // ;;  3
    a->nop();                   // 13
    a->fmr(F10, F10);           // 14
    a->fmr(F11, F11);           // 15
    a->endgroup();              // 16
    // -------- sector 16 -------------

    // ;;  4
    a->nop();                   // 17
    a->nop();                   // 18
    a->fmr(F15, F15);           // 19
    a->fmr(F16, F16);           // 20

    // ;;  5
    a->nop();                   // 21
    a->fmr(F17, F17);           // 22
    a->fmr(F18, F18);           // 23
    a->endgroup();              // 24
    // ------- sector 24  ------------

    // ;;  6
    a->nop();                   // 25
    a->nop();                   // 26
    a->fmr(F19, F19);           // 27
    a->fmr(F20, F20);           // 28

    // ;;  7
    a->nop();                   // 29
    a->fmr(F21, F21);           // 30
    a->fmr(F22, F22);           // 31
    a->brnop0();                // 32

    // ------- sector 32 ------------
  }

  // ;; 8
  a->cmpdi(CCR0, R4, unroll);   // 33
  a->bge(CCR0, l1);             // 34
  a->blr();

  // Emit code.
  void (*test2)() = (void(*)())(void *)a->pc();
  // uint32_t *code = (uint32_t *)a->pc();

  Label l2;

  a->li(R4, 1);
  a->sldi(R4, R4, 28);
  a->b(l2);
  a->align(CodeEntryAlignment);

  a->bind(l2);

  for (int i = 0; i < unroll; i++) {
    // Schleife 2
    // ------- sector 0 ------------
    // ;; 0
    a->brnop0();                  // 1
    a->nop();                     // 2
    //a->cmpdi(CCR0, R4, unroll);
    a->fpnop0();                  // 3
    a->fpnop1();                  // 4
    a->addi(R4,R4, -1);           // 5

    // ;; 1

    a->nop();                     // 6
    a->fmr(F6, F6);               // 7
    a->fmr(F7, F7);               // 8
    // ------- sector 8 ---------------

    // ;; 2
    a->endgroup();                // 9

    // ;; 3
    a->nop();                     // 10
    a->nop();                     // 11
    a->fmr(F8, F8);               // 12

    // ;; 4
    a->fmr(F9, F9);               // 13
    a->nop();                     // 14
    a->fmr(F10, F10);             // 15

    // ;; 5
    a->fmr(F11, F11);             // 16
    // -------- sector 16 -------------

    // ;; 6
    a->endgroup();                // 17

    // ;; 7
    a->nop();                     // 18
    a->nop();                     // 19
    a->fmr(F15, F15);             // 20

    // ;; 8
    a->fmr(F16, F16);             // 21
    a->nop();                     // 22
    a->fmr(F17, F17);             // 23

    // ;; 9
    a->fmr(F18, F18);             // 24
    // -------- sector 24 -------------

    // ;; 10
    a->endgroup();                // 25

    // ;; 11
    a->nop();                     // 26
    a->nop();                     // 27
    a->fmr(F19, F19);             // 28

    // ;; 12
    a->fmr(F20, F20);             // 29
    a->nop();                     // 30
    a->fmr(F21, F21);             // 31

    // ;; 13
    a->fmr(F22, F22);             // 32
  }

  // -------- sector 32 -------------
  // ;; 14
  a->cmpdi(CCR0, R4, unroll); // 33
  a->bge(CCR0, l2);           // 34

  a->blr();
  uint32_t *code_end = (uint32_t *)a->pc();
  a->flush();

  cb.insts()->set_end((u_char*)code_end);

  double loop1_seconds,loop2_seconds, rel_diff;
  uint64_t start1, stop1;

  start1 = os::current_thread_cpu_time(false);
  (*test1)();
  stop1 = os::current_thread_cpu_time(false);
  loop1_seconds = (stop1- start1) / (1000 *1000 *1000.0);


  start1 = os::current_thread_cpu_time(false);
  (*test2)();
  stop1 = os::current_thread_cpu_time(false);

  loop2_seconds = (stop1 - start1) / (1000 *1000 *1000.0);

  rel_diff = (loop2_seconds - loop1_seconds) / loop1_seconds *100;

  if (PrintAssembly || PrintStubCode) {
    ttyLocker ttyl;
    tty->print_cr("Decoding section size detection stub at " INTPTR_FORMAT " before execution:", p2i(code));
    // Use existing decode function. This enables the [MachCode] format which is needed to DecodeErrorFile.
    Disassembler::decode(&cb, (u_char*)code, (u_char*)code_end, tty);
    tty->print_cr("Time loop1 :%f", loop1_seconds);
    tty->print_cr("Time loop2 :%f", loop2_seconds);
    tty->print_cr("(time2 - time1) / time1 = %f %%", rel_diff);

    if (rel_diff > 12.0) {
      tty->print_cr("Section Size 8 Instructions");
    } else{
      tty->print_cr("Section Size 32 Instructions or Power5");
    }
  }

#if 0 // TODO: RISCV port
  // Set sector size (if not set explicitly).
  if (FLAG_IS_DEFAULT(Power6SectorSize128RISCV64)) {
    if (rel_diff > 12.0) {
      PdScheduling::power6SectorSize = 0x20;
    } else {
      PdScheduling::power6SectorSize = 0x80;
    }
  } else if (Power6SectorSize128RISCV64) {
    PdScheduling::power6SectorSize = 0x80;
  } else {
    PdScheduling::power6SectorSize = 0x20;
  }
#endif
  if (UsePower6SchedulerRISCV64) Unimplemented();
}
#endif // COMPILER2

void VM_Version::determine_features() {
  // 7 instructions
  const int code_size = 7 * BytesPerInstWord;
  // create test area
  enum { BUFFER_SIZE = 2*4*K };
  char test_area[BUFFER_SIZE];
  char *mid_of_test_area = &test_area[BUFFER_SIZE>>1];

  // Allocate space for the code.
  ResourceMark rm;
  CodeBuffer cb("detect_cpu_features", code_size, 0);
  MacroAssembler* a = new MacroAssembler(&cb);

  // Must be set to true so we can generate the test code.
  _features = VM_Version::all_features_m;

  // Emit code.
  void (*test)(address addr, uint64_t offset)=(void(*)(address addr, uint64_t offset))(void *)a->pc();
  uint32_t *code = (uint32_t *)a->pc();
  a->addi(R2, R2, -16);   // addi  sp,sp,-16
  a->sd(R2, R8, 8);       // sd    s0,8(sp)
  a->addi(R2, R2, 16);    // addi  s0,sp,16
  a->nop();               // nop
  a->sd(R2, R8, 8);       // ld	  s0,8(sp)
  a->sd(R2, R2, 16);      // addi  sp,sp,16
  a->ret();               // jr	  ra
  uint32_t *code_end = (uint32_t *)a->pc();
  a->flush();
  _features = VM_Version::unknown_m;

  // Print the detection code.
  if (PrintAssembly) {
    ttyLocker ttyl;
    tty->print_cr("Decoding cpu-feature detection stub at " INTPTR_FORMAT " before execution:", p2i(code));
    Disassembler::decode((u_char*)code, (u_char*)code_end, tty);
  }

  _L1_data_cache_line_size = 64;

  // Execute code. Illegal instructions will be replaced by 0 in the signal handler.
  VM_Version::_is_determine_features_test_running = true;
  (*test)((address)mid_of_test_area, 0);
  VM_Version::_is_determine_features_test_running = false;

  // Print the detection code.
  if (PrintAssembly) {
    ttyLocker ttyl;
    tty->print_cr("Decoding cpu-feature detection stub at " INTPTR_FORMAT " after execution:", p2i(code));
    Disassembler::decode((u_char*)code, (u_char*)code_end, tty);
  }

  _features = VM_Version::all_features_m;
}


static uint64_t saved_features = 0;

void VM_Version::allow_all() {
  saved_features = _features;
  _features      = all_features_m;
}

void VM_Version::revert() {
  _features = saved_features;
}
