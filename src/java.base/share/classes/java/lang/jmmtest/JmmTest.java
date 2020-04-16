package java.lang.jmmtest;

/**
 */
public class JmmTest {
    /**
     */
    public static volatile boolean actor1Started, actor2Started;
    /**
     */
    public static TestData[] testData;
    /**
     */
    public static ResultData[] resultData;
    /**
     */
    public static int iterations, errors, sideBySide, aOutrunsB, bOutrunsA;

    /**
     */
    public static void reset0() {
        iterations = 20000;
        actor1Started = false;
        actor2Started = false;
        testData = new TestData[iterations];
        resultData = new ResultData[iterations];
        for (int i = 0; i < iterations; i++) {
            testData[i] = new TestData();
            testData[i].a = 0;
            testData[i].b = 0;
            resultData[i] = new ResultData();
            resultData[i].a = 0;
            resultData[i].b = 0;
        }
    }

    /**
     */
    public static void actor1() {
        actor1Started = true;
        while(!actor2Started) {}
        for (int i = 0; i < iterations; i++) {
            testData[i].a = 1;
            resultData[i].a = testData[i].b;
        }
    }

    /**
     */
    public static void actor2() {
        actor2Started = true;
        while(!actor1Started) {}
        for (int i = 0; i < iterations; i++) {
            testData[i].b = 1;
            resultData[i].b = testData[i].a;
        }
    }

    /**
     */
    public static void calcResults() {
        errors = 0;
        sideBySide = 0;
        aOutrunsB = 0;
        bOutrunsA = 0;

        for (int i = 0; i < iterations; i++) {
            if (resultData[i].a == 1 && resultData[i].b == 1) {
                sideBySide = sideBySide + 1;
            }
            if (resultData[i].a == 0 && resultData[i].b == 0) {
                errors = errors + 1;
            }
            if (resultData[i].a == 1 && resultData[i].b == 0) {
                aOutrunsB = aOutrunsB + 1;
            }
            if (resultData[i].a == 0 && resultData[i].b == 1) {
                bOutrunsA = bOutrunsA + 1;
            }
        }
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