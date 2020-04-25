package java.lang.jmmtest;

/**
 */
public class CausalityTest3 {
	/**
	 */
	public static class ArrayData {
		/**
		 */
		public int[] a;
	}
    /**
     */
    public static volatile boolean actor1Started, actor2Started;
    /**
     */
    public static int[] x, y, result1, result2, result3;
    /**
     */
    public static ArrayData[] arrayData;
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
        arrayData = new ArrayData[iterations];
        result1 = new int[iterations];
        result2 = new int[iterations];
        result3 = new int[iterations];
		for(int i = 0; i < iterations; i++) {
			x[i] = 0;
			y[i] = 0;
			arrayData[i] = new ArrayData();
			arrayData[i].a = new int[2];
			arrayData[i].a[0] = 1;
			arrayData[i].a[1] = 2;
		}
    }

    /**
     */
    public static void actor1() {
        actor1Started = true;
        while(!actor2Started) {}

		for(int i = 0; i < iterations; i++) {
			result1[i] = x[i];
			arrayData[i].a[result1[i]] = 0;
			result2[i] = arrayData[i].a[0];
			y[i] = result2[i];
		}
    }

    /**
     */
    public static void actor2() {
        actor2Started = true;
        while(!actor1Started) {}

		for(int i = 0; i < iterations; i++) {
			result3[i] = y[i];
			x[i] = result3[i];
		}
    }

    /**
     */
    public static void calcResults() {
        errors = 0;
		for(int i = 0; i < iterations; i++) {
			if(result1[i] == 1 && result2[i] == 1 && result3[i] == 1) {
				errors++;
			}
		}
    }
}