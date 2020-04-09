#define test_li(x) TEST(#x) { li(Rtemplate0, x); expect(x); }

void test() {
	test_li(-128);
	test_li(0);
	test_li(-1);
	test_li(0b011111111111);
	test_li(-0b100000000000);

	test_li(2048);
	test_li(1 << 12);
	test_li(-4096);
	test_li((1L << 30) + 0b011111111111);
	test_li((1L << 30) + 0b111111111111);
	test_li(-(1L << 30) - 0b011111111111);
	test_li((1L << 31) - 1);
	test_li(-(1L << 31));

	long long long_max = ((unsigned long long) -1) >> 1;
	long long long_min = ((unsigned long long) 1) << 63;

	test_li((1L << 31) + 15);
	test_li(-(1L << 31) - 15);
	test_li((1L << 32) + 15);
	test_li(-(1L << 32) - 15);
	test_li(0b11111111111111111111111111100000);
	test_li((1L << 62));
	test_li(-(1L << 50));
	test_li((1L << 63));
	test_li(long_max);
	test_li(long_min);

#if 0

	for (long long i = 0; i < (1L << 32); i += (1L << 5)) {
		test_li(i);
	}

	for (long long i = (1L << 31) - 1; i < long_max - (1L << 37); i += (1L << 37)) {
		test_li(i);
	}


	for (long long i = 0; i > -(1L << 32); i -= (1L << 5)) {
		test_li(i);
	}

	for (long long i = (1L << 31) - 1; i > long_min; i -= (1L << 37)) {
		test_li(i);
	}

    TEST("LoadZero") {
        li(Rtemplate0, 0);
        expect(0);
    }
#endif
}
