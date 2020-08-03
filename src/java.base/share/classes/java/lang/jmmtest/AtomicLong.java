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
    public static long[] highest1, lowest1, highest2, lowest2;
    /**
     */
    public static int errors;

    /**
     */
    public static void reset0() {
        iterations = 30000;
        actor1Started = false;
        actor2Started = false;
        l = 0;
        long n = 56;
        key1 = n << 32;
        key1 |= n;
        n = 98;
        key2 = n << 32;
        key2 |= n;
        highest1 = new long[iterations];
        highest2 = new long[iterations];
        lowest1 = new long[iterations];
        lowest2 = new long[iterations];
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
		    highest1[i] = temp1;
		    lowest1[i] = temp3;
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
		    highest2[i] = temp1;
		    lowest2[i] = temp3;
		    l = key2;
		}
    }

    /**
     */
    public static void calcResults() {
        errors = 0;
		for(int i = 0; i < iterations; i++) {
		    if (highest1[i] != lowest1[i]) {
		        errors++;
		    }
		    if (highest2[i] != lowest2[i]) {
		        errors++;
		    }
		}
    }
}