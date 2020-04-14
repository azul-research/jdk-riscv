// load 64-bit immediate value
void li(Register d, long imm) {
  // tty->print_cr("li %s, 0x%lx at %p", d->name(), (unsigned long)imm, pc());

  if (-0x800 <= imm && imm < 0x800) {
    addi(d, R0_ZERO, imm);
    return;
  }

  long off = imm - (long)pc();

  if (INT32_MIN <= off && off <= INT32_MAX) {
    // load using AUIPC
    unsigned long uoff = off;
    unsigned long low = uoff & 0xfff;
    unsigned long high = (uoff >> 12) & 0xfffff;
    if (low >= 0x800) {
      ++high;
    }
    auipc(d, high);
    if (low) {
      addi(d, d, low);
    }
    return;
  }

  unsigned long uimm = imm;
  unsigned long value = uimm;

  unsigned long sha = 0; // shift amount for final value
  if (imm < INT32_MIN || imm > INT32_MAX) {
    while (!(value & 1)) {
      ++sha;
      value >>= 1;
    }
  }

  unsigned long low = value & 0xfff; // low section is 12 lowest bits
  unsigned long mid = value >> 12; // mid section is 20+ following bits
  if (low >= 0x800) {
    ++mid;
  }

  unsigned long shb = 0; // shift amount for mid section
  if (mid >= 0x100000 && !(mid >> 51)) {
    while (!(mid & 1)) {
      ++shb;
      mid >>= 1;
    }
  }

  unsigned long high = mid >> 20; // high section is remaining bits
  mid &= 0xfffff;
  if (mid >= 0x80000) {
    ++high;
  }
  high &= 0xfffffffful;

  if (!high) {
    // load mid
    if (mid) {
      lui(d, mid);
      if (shb) {
        slli(d, d, shb);
      }
    }

    // load low
    if (low) {
      addi(d, mid ? d : R0_ZERO, low);
      if (sha) {
        slli(d, d, sha);
      }
    }

    return;
  }

  // load negative constant the dumb way

  if (imm < 0) {
    li(d, -imm);
    sub(d, R0_ZERO, d);
    return;
  }

  // when all else fails, load by parts

  sha = 0;
  while (!(uimm & 1)) {
    ++sha;
    uimm >>= 1;
  }
  low = uimm & 0xffful;
  mid = uimm ^ low;
  if (low >= 0x800) {
    mid += 0x1000;
  }
  li(d, mid); // we zero lowest non-zero 12 bits, so recursion is finite
  if (low) {
    addi(d, d, low);
  }
  if (sha) {
    slli(d, d, sha);
  }
}
