/*
 * Copyright (c) 1997, 2010, Oracle and/or its affiliates. All rights reserved.
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
#include "interpreter/bytecodeHistogram.hpp"
#include "memory/resourceArea.hpp"
#include "runtime/os.hpp"
#include "utilities/growableArray.hpp"

// ------------------------------------------------------------------------------------------------
// Non-product code
#ifndef PRODUCT

// Implementation of BytecodeCounter

long  BytecodeCounter::_counter_value = 0;
jlong BytecodeCounter::_reset_time    = 0;


void BytecodeCounter::reset() {
  _counter_value = 0;
  _reset_time    = os::elapsed_counter();
}


double BytecodeCounter::elapsed_time() {
  return (double)(os::elapsed_counter() - _reset_time) / (double)os::elapsed_frequency();
}


double BytecodeCounter::frequency() {
  return (double)counter_value() / elapsed_time();
}


void BytecodeCounter::print() {
  tty->print_cr(
    "%ld bytecodes executed in %.1fs (%.3fMHz)",
    counter_value(),
    elapsed_time(),
    frequency() / 1000000.0
  );
}


// Helper class for sorting

class HistoEntry: public ResourceObj {
 private:
  int             _index;
  long            _count;

 public:
  HistoEntry(int index, long count)                        { _index = index; _count = count; }
  int             index() const                            { return _index; }
  long            count() const                            { return _count; }

  static long      compare(HistoEntry** x, HistoEntry** y)  { return (*x)->count() - (*y)->count(); }
};


// Helper functions

static GrowableArray<HistoEntry*>* sorted_array(long* array, int length) {
  GrowableArray<HistoEntry*>* a = new GrowableArray<HistoEntry*>(length);
  int i = length;
  while (i-- > 0) a->append(new HistoEntry(i, array[i]));
  a->sort(HistoEntry::compare);
  return a;
}


static long total_count(GrowableArray<HistoEntry*>* profile) {
  long sum = 0;
  int i = profile->length();
  while (i-- > 0) sum += profile->at(i)->count();
  return sum;
}


static const char* name_for(int i) {
  return Bytecodes::is_defined(i) ? Bytecodes::name(Bytecodes::cast(i)) : "xxxunusedxxx";
}


// Implementation of BytecodeHistogram

long BytecodeHistogram::_counters[Bytecodes::number_of_codes];


void BytecodeHistogram::reset() {
  int i = Bytecodes::number_of_codes;
  while (i-- > 0) _counters[i] = 0;
}


void BytecodeHistogram::print(float cutoff) {
  ResourceMark rm;
  GrowableArray<HistoEntry*>* profile = sorted_array(_counters, Bytecodes::number_of_codes);
  // print profile
  cutoff = -1;
  long tot     = total_count(profile);
  long abs_sum = 0;
  tty->cr();   //0123456789012345678901234567890123456789012345678901234567890123456789
  tty->print_cr("Histogram of %ld executed bytecodes:", tot);
  tty->cr();
  tty->print_cr("  absolute  relative  code    name");
  tty->print_cr("----------------------------------------------------------------------");
  int i = profile->length();
  while (i-- > 0) {
    HistoEntry* e = profile->at(i);
    long      abs = e->count();
    float     rel = abs * 100.0F / tot;
    if (cutoff <= rel) {
      tty->print_cr("%10ld  %7.2f%%    %02x    %s", abs, rel, e->index(), name_for(e->index()));
      abs_sum += abs;
    }
  }
  tty->print_cr("----------------------------------------------------------------------");
  float rel_sum = abs_sum * 100.0F / tot;
  tty->print_cr("%10ld  %7.2f%%    (cutoff = %.2f%%)", abs_sum, rel_sum, cutoff);
  tty->cr();
}


// Implementation of BytecodePairHistogram

int BytecodePairHistogram::_index;
long BytecodePairHistogram::_counters[BytecodePairHistogram::number_of_pairs];


void BytecodePairHistogram::reset() {
  _index = Bytecodes::_nop << log2_number_of_codes;

  int i = number_of_pairs;
  while (i-- > 0) _counters[i] = 0;
}


void BytecodePairHistogram::print(float cutoff) {
  ResourceMark rm;
  GrowableArray<HistoEntry*>* profile = sorted_array(_counters, number_of_pairs);
  // print profile
  long tot     = total_count(profile);
  long abs_sum = 0;
  tty->cr();   //0123456789012345678901234567890123456789012345678901234567890123456789
  tty->print_cr("Histogram of %ld executed bytecode pairs:", tot);
  tty->cr();
  tty->print_cr("  absolute  relative    codes    1st bytecode        2nd bytecode");
  tty->print_cr("----------------------------------------------------------------------");
  int i = profile->length();
  while (i-- > 0) {
    HistoEntry* e = profile->at(i);
    long       abs = e->count();
    float     rel = abs * 100.0F / tot;
    if (cutoff <= rel) {
      int   c1 = e->index() % number_of_codes;
      int   c2 = e->index() / number_of_codes;
      tty->print_cr("%10ld   %6.3f%%    %02x %02x    %-19s %s", abs, rel, c1, c2, name_for(c1), name_for(c2));
      abs_sum += abs;
    }
  }
  tty->print_cr("----------------------------------------------------------------------");
  float rel_sum = abs_sum * 100.0F / tot;
  tty->print_cr("%10ld   %6.3f%%    (cutoff = %.3f%%)", abs_sum, rel_sum, cutoff);
  tty->cr();
}


long DataCounter::method = 0;
long DataCounter::constMethod = 0;
long DataCounter::constMethod_codes = 0;
long DataCounter::InstanceKlass = 0;
long DataCounter::class_loader_data = 0;
long DataCounter::constantPool = 0;
long DataCounter::constantPool_Cache = 0;
long DataCounter::constantPool_Cache_tags = 0;
long DataCounter::constantPool_Cache_ResolvedReference = 0;
long DataCounter::thread_jvmtiThreadState = 0;
long DataCounter::thread_state = 0;
long DataCounter::thread_jvmci_alternate_call_target = 0;
long DataCounter::thread_callee_target = 0;
long DataCounter::thread_jni_environment = 0;
long DataCounter::access_flags = 0;
long DataCounter::size_of_parameters = 0;
long DataCounter::size_of_locals = 0;
long DataCounter::monitors_top = 0;
long DataCounter::interpreter_entry = 0;
long DataCounter::last_sp = 0;
long DataCounter::method_counters = 0;
long DataCounter::dispatch_table = 0;
long DataCounter::normal_table = 0;
long DataCounter::sender_sp = 0;
long DataCounter::call_stub = 0;
long DataCounter::normal_entry = 0;
long DataCounter::native_entry = 0;
long DataCounter::math_entry = 0;
long DataCounter::ref_get_entry = 0;
long DataCounter::synchronized_methods = 0;

long DataCounter::total_count() {
  return method
  + constMethod
  + constMethod_codes
  + InstanceKlass
  + class_loader_data
  + constantPool
  + constantPool_Cache
  + constantPool_Cache_tags
  + constantPool_Cache_ResolvedReference
  + thread_jvmtiThreadState
  + thread_state
  + thread_jvmci_alternate_call_target
  + thread_callee_target
  + thread_jni_environment
  + access_flags
  + size_of_parameters
  + size_of_locals
  + monitors_top
  + interpreter_entry
  + last_sp
  + method_counters
  + dispatch_table
  + normal_table
  + sender_sp
  ;
}

int ____log10(long data) {
  int res = 0;
  while (data) {
    res ++;
    data /= 10;
  }
  return res;
}

#define print_some__(data) tty->print_cr("%10ld            %5d   %s", data, ____log10(data), #data);
#define print_data__(data) if (data > 0) tty->print_cr("%10ld  %7.2f%%  %5d   %s", data, 100.0 * data / tot, ____log10(data), #data);
#define print_data_n(data, name) if (data > 0) tty->print_cr("%10ld  %7.2f%%  %5d   %s", data, 100.0 * data / tot, ____log10(data), name);

void DataCounter::print() {
  ResourceMark rm;
  long tot     = total_count();
  tty->cr();
  tty->print_cr("Histogram of %ld executed data getters:", tot);
  tty->cr();
  tty->print_cr("  absolute  relative  log10   name");
  tty->print_cr("----------------------------------------------------------------------");
  print_data__(method)
  print_data_n(constMethod, "method -> constMethod")
  print_data_n(constMethod_codes, "constMethod -> Codes")
  print_data_n(InstanceKlass, "constantPool -> InstanceKlass")
  print_data_n(class_loader_data, "InstanceKlass -> class loader data")
  print_data_n(constantPool, "constMethod -> constantPool")
  print_data_n(constantPool_Cache, "constantPool -> cache")
  print_data_n(constantPool_Cache_tags, "constantPool -> tags")
  print_data_n(constantPool_Cache_ResolvedReference, "cache -> resolved reference")
  print_data__(thread_state)
  print_data__(thread_jvmtiThreadState)
  print_data__(thread_jvmci_alternate_call_target)
  print_data__(thread_callee_target)
  print_data__(thread_jni_environment)
  print_data_n(access_flags, "method -> access_flags")
  print_data_n(size_of_parameters, "constMethod -> size_of_parameters")
  print_data_n(size_of_locals, "constMethod -> size_of_locals")
  print_data_n(monitors_top, "frame -> monitors")
  print_data_n(interpreter_entry, "method -> interpreter_entry")
  print_data_n(last_sp, "frame -> last_sp")
  print_data_n(method_counters, "metohd -> method counters")
  print_data__(dispatch_table)
  print_data__(normal_table)
  print_data_n(sender_sp, "frame -> sender_sp")
  tty->print_cr("----------------------------------------------------------------------");
  print_some__(call_stub)
  print_some__(normal_entry)
  print_some__(native_entry)
  print_some__(math_entry)
  print_some__(ref_get_entry)
  print_some__(synchronized_methods)
  tty->print_cr("----------------------------------------------------------------------");
  tty->cr();
}
#endif
