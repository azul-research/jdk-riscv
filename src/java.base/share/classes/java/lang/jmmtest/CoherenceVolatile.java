package java.lang.jmmtest;

/**
 */
public class CoherenceVolatile {
	/**
	 */
	public static class TestClass {
		/**
		 */
		volatile int x, y;
	}
    /**
     */
    public static volatile boolean actor1Started, actor2Finished;
    /**
     */
    public static TestClass data1, data2;
    /**
     */
    public static int iterations, errors;

    /**
     */
    public static void reset0() {
        iterations = 1000000;
        actor1Started = false;
        actor2Finished = false;
        errors = 0;
        data1 = new TestClass();
        data2 = data1;
        data1.x = 0;
        data1.y = 0;
    }

    /**
     */
    public static void actor1() {
        actor1Started = true;
        while(!actor2Finished) {
			data1.x++;
        }
    }

    /**
     */
    public static void actor2() {
        while(!actor1Started) {}

		TestClass locData1 = data1;
		TestClass locData2 = data2;
		int data1X, data2X, data1XAgain;
		for(int i = 0; i < iterations; i++) {
			data1X = locData1.x;
			data2X = locData2.x;
			data1XAgain = locData1.x;
			if (data2X > data1XAgain) {
				errors++;
			}
		}

        actor2Finished = true;
    }

    /**
     */
    public static void calcResults() {
    }
}