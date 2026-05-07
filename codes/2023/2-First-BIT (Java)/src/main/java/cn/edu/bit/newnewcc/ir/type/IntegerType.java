package cn.edu.bit.newnewcc.ir.type;

import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.exception.IllegalArgumentException;
import cn.edu.bit.newnewcc.ir.value.constant.ConstInteger;

/**
 * 整数类型
 */
public class IntegerType extends Type {
    private final int bitWidth;

    private IntegerType(int bitWidth) {
        this.bitWidth = bitWidth;
    }

    public int getBitWidth() {
        return bitWidth;
    }

    @Override
    protected String getTypeName_() {
        return String.format("i%d", bitWidth);
    }

    @Override
    public ConstInteger getZeroInitialization() {
        return ConstInteger.getInstance(bitWidth, 0);
    }

    @Override
    public long getSize() {
        return switch (bitWidth) {
            case 32 -> 4;
            case 64 -> 8;
            default -> throw new UnsupportedOperationException();
        };
    }

    private static class I1Holder {
        private static final IntegerType INSTANCE = new IntegerType(1);
    }

    private static class I32Holder {
        private static final IntegerType INSTANCE = new IntegerType(32);
    }

    private static class I64Holder {
        private static final IntegerType INSTANCE = new IntegerType(64);
    }

    public static IntegerType getInstance(int bitWidth) {
        return switch (bitWidth) {
            case 1 -> I1Holder.INSTANCE;
            case 32 -> I32Holder.INSTANCE;
            case 64 -> I64Holder.INSTANCE;
            default ->
                    throw new IllegalArgumentException(String.format("Bit width %d not suitable for integer type.", bitWidth));
        };
    }

    public static IntegerType getI1() {
        return getInstance(1);
    }

    public static IntegerType getI32() {
        return getInstance(32);
    }

    public static IntegerType getI64() {
        return getInstance(64);
    }

}
