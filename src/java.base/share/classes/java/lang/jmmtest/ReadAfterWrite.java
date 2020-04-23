package java.lang.jmmtest;

/**
 */
public class ReadAfterWrite {
    /**
     */
    public static volatile boolean actor1Started, actor2Started;
    /**
     */
    public static volatile int a, b;
    /**
     */
    public static int[] AA;
    /**
     */
    public static int[] BB;
    /**
     */
    public static int iterations, errors;

    /**
     */
    public static void reset0() {
        iterations = 30000;
        actor1Started = false;
        actor2Started = false;
        AA = new int[iterations];
        BB = new int[iterations];
        a = 0;
        b = 0;
        for (int i = 0; i < iterations; i++) {
            AA[i] = 0;
            BB[i] = 0;
        }
    }

    /**
     */
    public static void actor1() {
        actor1Started = true;
        while(!actor2Started) {}
        for (int i = 0; i < iterations; i++) {
            a = i;
            AA[i] = b;
        }
    }

    /**
     */
    public static void actor2() {
        actor2Started = true;
        while(!actor1Started) {}
        for (int i = 0; i < iterations; i++) {
            b = i;
            BB[i] = a;
        }
    }

    /**
     */
    public static void calcResults() {
        errors = 0;
        for (int i = 100; i < iterations; i++) {
            int j = AA[i] + 1;
            if (100 < j && j < iterations && BB[j] < i) {
                errors++;
            }
        }
    }
}