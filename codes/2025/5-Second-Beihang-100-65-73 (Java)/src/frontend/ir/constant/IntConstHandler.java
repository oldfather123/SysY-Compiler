package frontend.ir.constant;

public class IntConstHandler implements ConstHandler<Integer> {
    public Integer getValue(IRConst constant) {
        return constant.getConstVal().intValue();
    }

    public IRConst makeConst(Integer value) {
        return new IntConst(value);
    }

    public IRConst makeZero() {
        return new IntConst(0);
    }

    public boolean isZero(Integer value) {
        return value == 0;
    }

    public boolean isOne(Integer value) {
        return value == 1;
    }

    public Integer add(Integer a, Integer b) {
        return a + b;
    }

    public Integer sub(Integer a, Integer b) {
        return a - b;
    }

    public Integer mul(Integer a, Integer b) {
        return a * b;
    }

    public Integer neg(Integer a) {
        return -a;
    }
}
