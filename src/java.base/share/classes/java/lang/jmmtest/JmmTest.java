package java.lang.jmmtest;

/**
 */
public class JmmTest {
	/**
	 */
	public static volatile boolean start0, aFinished, bFinished, calcFinished;
	/**
	 */
    public static TestData[] testData;
	/**
	 */
    public static ResultData[] resultData;
    /**
     */
    public static int i0, i1, i2, i3, iterations, errors, sideBySide, aOutrunsB, bOutrunsA;
    /**
     */
    public static volatile int t0, t1, t3;

    /**
     */
    public static void initt() {
        testData = new TestData[1];
        testData[0] = new TestData();
        testData[0].a = 12;
        testData[0].b = 111;
        t3 = 777;
        while (true) {}
    }

    /**
     */
    public static void sett() {
        t0 = testData[0].a;
        t1 = testData[0].b;
        while (true) {}
    }

    /**
     */
    public static void reset0() {
        iterations = 10000;
        calcFinished = false;
        aFinished = false;
        bFinished = false;
        testData = new TestData[iterations];
        resultData = new ResultData[iterations];
        for (i0 = 0; i0 < iterations; i0 = i0 + 1) {
            testData[i0] = new TestData();
            testData[i0].a = 0;
            testData[i0].b = 0;
            resultData[i0] = new ResultData();
            resultData[i0].a = 0;
            resultData[i0].b = 0;
        }
        start0 = true;
        while (true) {}
	}

    /**
     */
    public static void actor0() {
        start0 = true;
    }

    /**
     */
    public static void actor1() {
        while(!start0) {}
        for (i1 = 0; i1 < iterations; i1 = i1 + 1) {
            testData[i1].a = 1;
            resultData[i1].a = testData[i1].b;
        }
        aFinished = true;
        while (true) {}
    }

    /**
     */
    public static void actor2() {
        while(!start0) {}
        for (i2 = 0; i2 < iterations; i2 = i2 + 1) {
            testData[i2].b = 1;
            resultData[i2].b = testData[i2].a;
        }
        bFinished = true;
        while (true) {}
    }

    /**
     */
    public static void calcResults() {
        while (!aFinished || !bFinished) {}
        errors = 0;
        sideBySide = 0;
        aOutrunsB = 0;
        bOutrunsA = 0;

        for (i3 = 0; i3 < iterations; i3 = i3 + 1) {
            if (resultData[i3].a == 1 && resultData[i3].b == 1) {
                sideBySide = sideBySide + 1;
            }
            if (resultData[i3].a == 0 && resultData[i3].b == 0) {
                errors = errors + 1;
            }
            if (resultData[i3].a == 1 && resultData[i3].b == 0) {
                aOutrunsB = aOutrunsB + 1;
            }
            if (resultData[i3].a == 0 && resultData[i3].b == 1) {
                bOutrunsA = bOutrunsA + 1;
            }
        }

        calcFinished = true;
        while (true) {}
    }

    /**
     */
    static public class ResultData {
    	/**
    	 */
        public int a, b;
    }

    /**
     */
    static public class TestData {
    	/**
    	 */
        public volatile int a, b;
    }
}