package midend.range;

public class RangeMath {
    /* ===================== Arithmetic helpers ===================== */

    public static int incCap(int x) {
        return x == Integer.MAX_VALUE ? x : x + 1;
    }

    public static int decCap(int x) {
        return x == Integer.MIN_VALUE ? x : x - 1;
    }

    public static int safeAdd(int a, int b, boolean forMin) {
        try {
            return Math.addExact(a, b);
        } catch (ArithmeticException e) {
            return forMin ? Integer.MIN_VALUE : Integer.MAX_VALUE;
        }
    }

    public static int safeSub(int a, int b, boolean forMin) {
        try {
            return Math.subtractExact(a, b);
        } catch (ArithmeticException e) {
            return forMin ? Integer.MIN_VALUE : Integer.MAX_VALUE;
        }
    }

    public static int safeMul(int a, int b, boolean forMin) {
        try {
            return Math.multiplyExact(a, b);
        } catch (ArithmeticException e) {
            return forMin ? Integer.MIN_VALUE : Integer.MAX_VALUE;
        }
    }

    public static int safeDiv(int a, int b) {
        if (b == 0) return 0;
        return a / b;
    }

    public static Range remRange(Range r1, Range r2) {
        int M = Math.max(Math.abs(r2.min), Math.abs(r2.max));
        if (M == 0) return Range.top();
        int lo = r1.min < 0 ? -M + 1 : 0;
        int hi = r1.max > 0 ? M - 1 : 0;
        return new Range(lo, hi);
    }

    public static Range divRange(Range r1, Range r2) {
        if (r2.min <= 0 && r2.max >= 0) return Range.top(); // divisor may be 0
        int c1 = safeDiv(r1.min, r2.min), c2 = safeDiv(r1.min, r2.max);
        int c3 = safeDiv(r1.max, r2.min), c4 = safeDiv(r1.max, r2.max);
        int mn = Math.min(Math.min(c1, c2), Math.min(c3, c4));
        int mx = Math.max(Math.max(c1, c2), Math.max(c3, c4));
        return new Range(mn, mx);
    }

}
