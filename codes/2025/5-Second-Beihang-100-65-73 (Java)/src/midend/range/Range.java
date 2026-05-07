package midend.range;

public class Range {
    public final int min;
    public final int max;

    public Range(int min, int max) {
        this.min = min;
        this.max = max;
    }

    private static final Range TOP = new Range(Integer.MIN_VALUE, Integer.MAX_VALUE);
    private static final Range BOTTOM = new Range(1, 0); // min>max denotes ⊥

    // 小整数常量池
    private static final int POOL_LO = -128;
    private static final int POOL_HI = 127;
    private static final Range[] POOL = new Range[POOL_HI - POOL_LO + 1];

    static {
        for (int i = POOL_LO; i <= POOL_HI; i++) {
            POOL[i - POOL_LO] = new Range(i, i);
        }
    }

    public static Range top() {
        return TOP;
    }

    public static Range bottom() {
        return BOTTOM;
    }

    public boolean isTop() {
        return this == TOP || (min == Integer.MIN_VALUE && max == Integer.MAX_VALUE);
    }

    public boolean isBottom() {
        return min > max;
    }

    public static Range constRange(int c) {
        if (c >= POOL_LO && c <= POOL_HI) return POOL[c - POOL_LO];
        return new Range(c, c);
    }

    /**
     * 统一工厂：集中做 TOP/BOTTOM/单点区间的规范化与驻留
     */
    public static Range of(int lo, int hi) {
        if (lo > hi) return BOTTOM;
        if (lo == Integer.MIN_VALUE && hi == Integer.MAX_VALUE) return TOP;
        if (lo == hi) return constRange(lo);
        return new Range(lo, hi);
    }

    public Range union(Range o) {
                /*if (this.isBottom()) return o;
                if (o.isBottom()) return this;
                return new Range(Math.min(min, o.min), Math.max(max, o.max));*/
        if (this == o) return this;
        if (this.isBottom()) return o;
        if (o.isBottom()) return this;
        if (this.isTop() || o.isTop()) return TOP;
        int nmin = Math.min(min, o.min);
        int nmax = Math.max(max, o.max);
        if (nmin == min && nmax == max) return this;
        if (nmin == o.min && nmax == o.max) return o;
        return of(nmin, nmax);
    }

    public Range intersect(Range o) {
                /*if (this.isBottom() || o.isBottom()) return bottom();
                int nmin = Math.max(min, o.min);
                int nmax = Math.min(max, o.max);
                if (nmin > nmax) return bottom();
                return new Range(nmin, nmax);*/
        if (this == o) return this;
        if (this.isBottom() || o.isBottom()) return BOTTOM;
        int nmin = Math.max(min, o.min);
        int nmax = Math.min(max, o.max);
        if (nmin > nmax) return BOTTOM;
        if (nmin == min && nmax == max) return this;
        if (nmin == o.min && nmax == o.max) return o;
        return of(nmin, nmax);
    }

    /**
     * Widening (classic): expand towards +/-INF when bounds grow
     */
    public Range widen(Range next) {
            /*if (this.isBottom()) return next; // first hit
            int nmin = next.min < this.min ? Integer.MIN_VALUE : this.min;
            int nmax = next.max > this.max ? Integer.MAX_VALUE : this.max;
            return new Range(nmin, nmax);*/
        if (this.isBottom()) return next; // first hit
        int nmin = next.min < this.min ? Integer.MIN_VALUE : this.min;
        int nmax = next.max > this.max ? Integer.MAX_VALUE : this.max;
        if (nmin == this.min && nmax == this.max) return this;
        return of(nmin, nmax);
    }

    @Override
    public boolean equals(Object o) {
        if (!(o instanceof Range r)) return false;
        if (this.isBottom() && r.isBottom()) return true;
        return this.min == r.min && this.max == r.max;
    }

    @Override
    public String toString() {
        return isBottom() ? "⊥" : "[" + min + "," + max + "]";
    }
}