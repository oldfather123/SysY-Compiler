package frontend.ir.constant;

public interface ConstHandler<T> {
    T getValue(IRConst constant);
    IRConst makeConst(T value);
    IRConst makeZero();
    boolean isZero(T value);
    boolean isOne(T value);
    T add(T a, T b);
    T sub(T a, T b);
    T mul(T a, T b);
    T neg(T a);
}