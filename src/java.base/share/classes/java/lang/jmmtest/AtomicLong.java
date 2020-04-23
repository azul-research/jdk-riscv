package java.lang.jmmtest;

/**
 */
public class AtomicLong {
    /**
     */
    public static volatile boolean actor1Started, actor2Started;
    /**
     */
    public static volatile long l;
    /**
     */
    public static long key1, key2;
    /**
     */
    public static int iterations;
    /**
     */
    public static volatile int errors;

    /**
     */
    public static void reset0() {
        iterations = 30000;
        actor1Started = false;
        actor2Started = false;
        l = 0;
        errors = 0;
        key1 = 231 << 32 | 231;
        key2 = -56 << 32 | -56;
    }

    /**
     */
    public static void actor1() {
        actor1Started = true;
        while(!actor2Started) {}

		for(int i = 0; i < iterations; i++) {
		    long temp = l;
		    long temp1 = temp >>> 32;
		    long temp2 = (temp << 32) >>> 32;
		    long temp3 = 0xffffffffL & temp;
		    if (temp2 != temp3) {
		        errors++;
		    }
		    if (temp1 != temp3) {
		        errors++;
		    }
		    l = key1;
		}
    }

    /**
     */
    public static void actor2() {
        actor2Started = true;
        while(!actor1Started) {}

		for(int i = 0; i < iterations; i++) {
		    long temp = l;
		    long temp1 = temp >>> 32;
		    long temp2 = (temp << 32) >>> 32;
		    long temp3 = 0xffffffffL & temp;
		    if (temp2 != temp3) {
		        errors++;
		    }
		    if (temp1 != temp3) {
		        errors++;
		    }
		    l = key2;
		}
    }

    /**
     */
    public static void calcResults() {
    }
}