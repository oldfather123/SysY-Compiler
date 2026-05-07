package cn.edu.bit.newnewcc.backend.asm.util;

public class ComparablePair<A extends Comparable<A>, B extends Comparable<B>> implements Comparable<ComparablePair<A, B>> {
    public A a;
    public B b;

    public ComparablePair(A a, B b) {
        this.a = a;
        this.b = b;
    }

    @Override
    public int compareTo(ComparablePair<A, B> o) {
        if (a.compareTo(o.a) != 0) {
            return a.compareTo(o.a);
        }
        return b.compareTo(o.b);
    }

    @Override
    public String toString() {
        return "<" + a.toString() + "," + b.toString() + ">";
    }
}
