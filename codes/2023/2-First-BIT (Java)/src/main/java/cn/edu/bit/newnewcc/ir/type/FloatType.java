package cn.edu.bit.newnewcc.ir.type;

import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.exception.IllegalBitWidthException;
import cn.edu.bit.newnewcc.ir.value.constant.ConstFloat;

/**
 * 浮点类型
 * <p>
 * 在SysY中目前只有float类型，但是我们还是预留了double,float128等类型的接口
 */
public class FloatType extends Type {
    private final int bitWidth;

    private FloatType(int bitWidth) {
        this.bitWidth = bitWidth;
    }

    @Override
    protected String getTypeName_() {
        if (bitWidth != 32)
            throw new IllegalBitWidthException();

        return "float";
    }

    @Override
    public ConstFloat getZeroInitialization() {
        if (bitWidth != 32)
            throw new IllegalBitWidthException();

        return ConstFloat.getInstance(0);
    }

    @Override
    public long getSize() {
        if (bitWidth != 32)
            throw new UnsupportedOperationException();

        return 4;
    }

    private static class FloatHolder {
        private static final FloatType INSTANCE = new FloatType(32);
    }

    public static FloatType getFloat() {
        return FloatHolder.INSTANCE;
    }
}
