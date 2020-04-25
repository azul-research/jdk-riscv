package java.lang.jmmtest;

/**
 */
public class CausalityTest4 {
    /**
     */
    public static volatile boolean actor1Started, actor2Started;
    /**
     */
    public static int[] x, y, result1, result2;
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
        x = new int[iterations];
        y = new int[iterations];
        result1 = new int[iterations];
        result2 = new int[iterations];
		for(int i = 0; i < iterations; i++) {
			x[i] = 0;
			y[i] = 0;
		}
    }

    /**
     */
    public static void actor1() {
        actor1Started = true;
        while(!actor2Started) {}

		for(int i = 0; i < iterations; i++) {
			result1[i] = x[i];
			if (result1[i] == 1) {
				y[i] = 1;
			}
		}
    }

    /**
     */
    public static void actor2() {
        actor2Started = true;
        while(!actor1Started) {}

		for(int i = 0; i < iterations; i++) {
			result2[i] = y[i];
			if (result2[i] == 1) {
				x[i] = 1;
			}
		}
    }

    /**
     */
    public static void calcResults() {
        errors = 0;
		for(int i = 0; i < iterations; i++) {
			if(!(result1[i] == 0 && result2[i] == 0)) {
				errors++;
			}
		}
    }
}