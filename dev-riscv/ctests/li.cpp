#include <iostream>
#include <string>
#include <bitset>
#include <climits>
#include <assert.h>

using namespace std;

struct Register {
	long long val = 0;
};

Register R0_ZERO = {0};

void show_binrep(long long t) {
    cout << bitset<64>(t) << endl;
};


void slli(Register &d, Register &s, int imm) {
  d.val = s.val << imm;
}

long long signed12(long long v) {
  return (long long) (-((v >> 11) & 1) << 12) | v;
}

// imm 20 bits
void lui(Register &d, int imm) {
	d.val = (imm << 12) & 0xffffffff;
}

// imm 12 bits
void addi(Register &d, Register &s, int imm) {
	d.val = s.val + signed12(imm);
}

// imm 12 bits
void andi(Register &d, Register &s, int imm) {
  d.val = s.val | signed12(imm);
}

void li_32(Register &d, int imm) {
  short l12 = imm & 0x0fff; // lowest 12 bit of immediate.
  int sign12 = (unsigned short)(l12 << 4) >> 15;
  int rem12 = (imm >> 12) + sign12; // Compensation for sign extend.
  if (rem12 == 0) {
    addi(d, R0_ZERO, imm);
    return;
  }

  int signImm = (unsigned long) imm >> 63;

  lui(d, rem12); // Put upper 20 bits and places zero in the lowest 12 bits.
  if (l12 != 0) addi(d, d, l12);
}

void li(Register &d, long imm) { // TODO optimize
  unsigned long uimm = imm;

  // Accurate copying by 11 bits.
  int remBit = ((uimm & 0xffffffff00000000) != 0) ? 53 : 21;
  short part = (uimm >> remBit) & 0x7FF;
  addi(d, R0_ZERO, part);
  slli(d, d, 11);

  for (remBit -= 11; remBit > 0; remBit -= 11) {
    part = (uimm >> remBit) & 0x7FF;
    addi(d, d, part);
    slli(d, d, remBit >= 11 ? 11 : remBit);
  }

  part = uimm & ((1 << (remBit + 11)) - 1);
  addi(d, d, part);
}

bool TEST_VERBOSE = true;

void test(Register &r, long long v) {
	li(r, v);
	if (r.val == v) {
		if (TEST_VERBOSE) {
      cout << "\t" << v << "\tOK" << endl;
    }
	} else {
		cout << "ERROR. Expected: " << v << " - "; show_binrep(v);
		cout << "       Actual:   " << r.val << " - "; show_binrep(r.val); cout << endl;
    exit(1);
	}
}

void section(string &&s) {
	cout << endl << "Section: " << s << endl << endl;
}

void testADDI(Register d, int v) {
  andi(d, R0_ZERO, v);
  // assert(d.val == v);
  cout << d.val << endl;
}


int main() {
	Register r;

  // testADDI(r, 1);
  // testADDI(r, -1);
  // testADDI(r, 0b011111111111);
  // testADDI(r, 0b111111111111);

	section("12 bit");
	
	test(r, 128);
	test(r, -128);
	test(r, 0);
	test(r, -1);
	test(r, 0b011111111111);
	test(r, -0b100000000000);

	section("32 bit");

	test(r, 2048);
  test(r, 1 << 12);
  test(r, -4096);
	test(r, (1L << 30) + 0b011111111111);
  test(r, (1L << 30) + 0b111111111111);
  test(r, -(1L << 30) - 0b011111111111);
  test(r, (1L << 31) - 1);
  test(r, -(1L << 31));

  section("64 bit");

  long long long_max = ((unsigned long long) -1) >> 1;
  long long long_min = ((unsigned long long) 1) << 63;

  test(r, (1L << 31) + 15);
  test(r, -(1L << 31) - 15);
  test(r, (1L << 32) + 15);
  test(r, -(1L << 32) - 15);
  test(r, 0b11111111111111111111111111100000);
  test(r, (1L << 62));
  test(r, -(1L << 50));
  test(r, (1L << 63));
  test(r, long_max);
  test(r, long_min);


  TEST_VERBOSE = false;


  for (long long i = 0; i < (1L << 32); i += (1L << 5)) {
    test(r, i);
  }

  for (long long i = (1L << 31) - 1; i < long_max - (1L << 37); i += (1L << 37)) {
    test(r, i);
  }


  cout << "positive tested" << endl;


  for (long long i = 0; i > -(1L << 32); i -= (1L << 5)) {
    test(r, i);
  }

  for (long long i = (1L << 31) - 1; i > long_min; i -= (1L << 37)) {
    test(r, i);
  }

  cout << "negative tested" << endl;

	return 0;
}