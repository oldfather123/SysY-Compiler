package frontend.ir.constant;

public class FloatConstHandler implements ConstHandler<Float> {
    public Float getValue(IRConst constant) {
        return constant.getConstVal().floatValue();
    }

    public IRConst makeConst(Float value) {
        return new FloatConst(value);
    }

    public IRConst makeZero() {
        return new FloatConst(0.0f);
    }

    public boolean isZero(Float value) {
        return value == 0.0f; // 注意浮点比较
    }

    public boolean isOne(Float value) {
        return value == 1.0f;
    }

    public Float add(Float a, Float b) {
        return a + b;
    }

    public Float sub(Float a, Float b) {
        return a - b;
    }

    public Float mul(Float a, Float b) {
        return a * b;
    }

    public Float neg(Float a) {
        return -a;
    }
}
