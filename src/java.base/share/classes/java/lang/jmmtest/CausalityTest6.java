package java.lang.jmmtest;

/**
 */
public class CausalityTest6 {
	/**
	 */
	public static class VolatileInt {
		/**
		 */
		public volatile int data;
	}
    /**
     */
    public static volatile boolean actor1Started, actor2Started, actor3Started;
    /**
     */
    public static int[] a, b, result0, result1, result2, result3;
    /**
     */
    public static VolatileInt[] x, y;
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
        actor3Started = false;
        a = new int[iterations];
        b = new int[iterations];
        x = new VolatileInt[iterations];
        y = new VolatileInt[iterations];
        result0 = new int[iterations];
        result1 = new int[iterations];
        result2 = new int[iterations];
        result3 = new int[iterations];
		for(int i = 0; i < iterations; i++) {
			a[i] = 0;
			b[i] = 0;
			x[i] = new VolatileInt();
			x[i].data = 0;
			y[i] = new VolatileInt();
			y[i].data = 0;
		}
    }

    /**
     */
    public static void actor1() {
        actor1Started = true;
        while(!actor2Started || !actor3Started) {}

		for(int i = 0; i < iterations; i++) {
			result0[i] = x[i].data;
			if (result0[i] == 1) {
				result1[i] = a[i];
			} else {
				result1[i] = 0;
			}
			if (result1[i] == 0) {
				y[i].data = 1;
			} else {
				b[i] = 1;
			}
		}
    }

    /**
     */
    public static void actor2() {
        actor2Started = true;
        while(!actor1Started || !actor3Started) {}

		for(int i = 0; i < iterations; i++) {
			do {
				result2[i] = y[i].data;
				result3[i] = b[i];
			} while(result2[i] + result3[i] == 0);
			a[i] = 1;
		}
    }

    /**
     */
    public static void actor3() {
        actor3Started = true;
        while(!actor1Started || !actor2Started) {}

		for(int i = 0; i < iterations; i++) {
			x[i].data = 1;
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