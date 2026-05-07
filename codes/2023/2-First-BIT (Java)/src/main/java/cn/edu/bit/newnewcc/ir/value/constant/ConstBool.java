package cn.edu.bit.newnewcc.ir.value.constant;

public class ConstBool extends ConstInteger {

    private final boolean value;

    private ConstBool(boolean value) {
        super(1);
        this.value = value;
    }

    @Override
    public boolean isFilledWithZero() {
        return !value;
    }

    @Override
    public String getValueName() {
        return String.valueOf(value);
    }

    public boolean getValue() {
        return value;
    }

    private static class TrueHolder {
        private static final ConstBool INSTANCE = new ConstBool(true);
    }

    private static class FalseHolder {
        private static final ConstBool INSTANCE = new ConstBool(false);
    }

    public static ConstBool getInstance(boolean value) {
        return value ? TrueHolder.INSTANCE : FalseHolder.INSTANCE;
    }
}
