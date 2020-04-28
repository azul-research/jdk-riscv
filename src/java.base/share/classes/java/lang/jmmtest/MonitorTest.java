package java.lang.jmmtest;

/**
 */
public class MonitorTest {
    /**
     */
    public static volatile boolean actor1Started, actor2Started;
    /**
     */
    public static int iterations;
    /**
     */
    public static int[] a;
    /**
     */
    public static Object monitor;
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
        monitor = new Object();
		for(int i = 0; i < iterations; i++) {
			a[i] = 0;
		}
    }

    /**
     */
    public static void actor1() {
        actor1Started = true;
        while(!actor2Started) {}

		for(int i = 0; i < iterations; i++) {
		    synchronized(monitor) {
		        a[i]++;
		        a[i]++;
		        a[i]++;
		        a[i]++;
		        a[i]++;
		    }
		}
    }

    /**
     */
    public static void actor2() {
        actor2Started = true;
        while(!actor1Started) {}

		for(int i = 0; i < iterations; i++) {
		    synchronized(monitor) {
		        a[i]++;
		        a[i]++;
		        a[i]++;
		        a[i]++;
		        a[i]++;
		    }
		}
    }

    /**
     */
    public static void calcResults() {
        errors = 0;
		for(int i = 0; i < iterations; i++) {
		    if (a[i] != 10) {
		        errors++;
		    }
		}
    }
}