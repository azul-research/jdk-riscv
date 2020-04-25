package java.lang.jmmtest;

/**
 */
public class CausalityTest1 {
    /**
     */
    public static volatile boolean actor1Started, actor2Started, actor3Started, actor4Started;
    /**
     */
    public static int[] x, y, z, result1, result2, result3;
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
        actor4Started = false;
        x = new int[iterations];
        y = new int[iterations];
        z = new int[iterations];
        result1 = new int[iterations];
        result2 = new int[iterations];
        result3 = new int[iterations];
		for(int i = 0; i < iterations; i++) {
			x[i] = 0;
			y[i] = 0;
			z[i] = 0;
		}
    }

    /**
     */
    public static void actor1() {
        actor1Started = true;
        while(!actor2Started || !actor3Started || !actor4Started) {}

		for(int i = 0; i < iterations; i++) {
			result1[i] = x[i];
			y[i] = result1[i];
		}
    }

    /**
     */
    public static void actor2() {
        actor2Started = true;
        while(!actor1Started || !actor3Started || !actor4Started) {}

		for(int i = 0; i < iterations; i++) {
			result2[i] = y[i];
			x[i] = result2[i];
		}
    }

    /**
     */
    public static void actor3() {
        actor3Started = true;
        while(!actor2Started || !actor1Started || !actor4Started) {}

		for(int i = 0; i < iterations; i++) {
			z[i] = 1;
		}
    }

    /**
     */
    public static void actor4() {
        actor4Started = true;
        while(!actor2Started || !actor3Started || !actor1Started) {}

		for(int i = 0; i < iterations; i++) {
			result3[i] = z[i];
			x[i] = result3[i];
		}
    }

    /**
     */
    public static void calcResults() {
        errors = 0;
		for(int i = 0; i < iterations; i++) {
			if(result1[i] == 1 && result2[i] == 1 && result3[i] == 0) {
				errors++;
			}
		}
    }
}