package java.lang.jmmtest;

/**
 */
public class HB2 {
	/**
	 */
	public static class VolatileInt {
		/**
		 */
		public volatile int data;
	}
    /**
     */
    public static volatile boolean actor1Started, actor2Started;
    /**
     */
    public static int[] nonvol, volRead, nonvolRead;
    /**
     */
    public static VolatileInt[] vol;
    /**
     */
    public static int iterations;
    /**
     */
    public static int errors, happenedBefore;

    /**
     */
    public static void reset0() {
        iterations = 30000;
        actor1Started = false;
        actor2Started = false;
        nonvol = new int[iterations];
        nonvolRead = new int[iterations];
        volRead = new int[iterations];
        vol = new VolatileInt[iterations];
		for(int i = 0; i < iterations; i++) {
			vol[i] = new VolatileInt();
			vol[i].data = 0;
			nonvol[i] = 0;
		}
    }

    /**
     */
    public static void actor1() {
        actor1Started = true;
        while(!actor2Started) {}

		for(int i = 0; i < iterations; i++) {
			nonvolRead[i] = nonvol[i];
			vol[i].data = i + 1;
		}
    }

    /**
     */
    public static void actor2() {
        actor2Started = true;
        while(!actor1Started) {}

		for(int i = 0; i < iterations; i++) {
			volRead[i] = vol[i].data;
			nonvol[i] = i + 1;
		}
    }

    /**
     */
    public static void calcResults() {
        errors = 0;
        happenedBefore = 0;
		for(int i = 0; i < iterations; i++) {
			if (volRead[i] != 0) {
				happenedBefore++;
			}
			if (volRead[i] == i + 1 && nonvolRead[i] == i + 1) {
				errors++;
			}
		}
    }
}