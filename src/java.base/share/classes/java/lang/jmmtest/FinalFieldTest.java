package java.lang.jmmtest;

/**
 */
public class FinalFieldTest {
    /**
     */
    public static volatile boolean actor1Started, actor2Started;
    /**
     */
    public static int iterations;
    /**
     */
    public static volatile int errors;
    /**
     */
    public static long finalFieldValue;
    /**
     */
    public static FinalFieldClass[] testArray;

    /**
     */
    public static void reset0() {
        iterations = 30000;
        actor1Started = false;
        actor2Started = false;
        finalFieldValue = -12;
        testArray = new FinalFieldClass[iterations];
        errors = 0;
    }

    /**
     */
    public static void actor1() {
        actor1Started = true;
        while(!actor2Started) {}

		for(int i = 0; i < iterations; i++) {
		    testArray[i] = new FinalFieldClass();
		}
    }

    /**
     */
    public static void actor2() {
        actor2Started = true;
        while(!actor1Started) {}

		for(int i = 0; i < iterations; i++) {
			FinalFieldClass ffo = testArray[i];
			while (ffo == null) {
				ffo = testArray[i];
			}
			if (ffo.a != finalFieldValue) {
				errors++;
			}
		}
    }

    /**
     */
    public static void calcResults() {
    }

    /**
     */
    static public class FinalFieldClass {
        /**
         */
        public final long a;

		/**
		 */
        public FinalFieldClass() {
            a = FinalFieldTest.finalFieldValue;
        }
    }
}