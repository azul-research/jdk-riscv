package java.lang.jmmtest;

/**
 */
public class CausalityTest5 {
	/**
	 */
	public static class VolatileInt {
		/**
		 */
		public volatile int y;
	}
    /**
     */
    public static volatile boolean actor1Started, actor2Started;
    /**
     */
    public static int[] a, b, result1, result2, result3;
    /**
     */
    public static VolatileInt[] ay;
    /**
     */
    public static int iterations;
    /**
     */
    public static int errors;

    /**
     */
    public static void reset0() {
        iterations = 30000;
        actor1Started = false;
        actor2Started = false;
        a = new int[iterations];
        b = new int[iterations];
        ay = new VolatileInt[iterations];
        result1 = new int[iterations];
        result2 = new int[iterations];
        result3 = new int[iterations];
		for(int i = 0; i < iterations; i++) {
			a[i] = 0;
			b[i] = 0;
			ay[i] = new VolatileInt();
			ay[i].y = 0;
		}
    }

    /**
     */
    public static void actor1() {
        actor1Started = true;
        while(!actor2Started) {}

		for(int i = 0; i < iterations; i++) {
			result1[i] = a[i];
			if (result1[i] == 0) {
				ay[i].y = 1;
			} else {
				b[i] = 1;
			}
		}
    }

    /**
     */
    public static void actor2() {
        actor2Started = true;
        while(!actor1Started) {}

		for(int i = 0; i < iterations; i++) {
			do {
				result2[i] = ay[i].y;
				result3[i] = b[i];
			} while(result2[i] + result3[i] == 0);
			a[i] = 1;
		}
    }

    /**
     */
    public static void calcResults() {
        errors = 0;
		for(int i = 0; i < iterations; i++) {
			if(result1[i] != 0) {
				errors++;
			}
		}
    }
}