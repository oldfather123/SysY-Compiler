package cn.edu.bit.newnewcc.ir.value.constant;

import cn.edu.bit.newnewcc.ir.type.VoidType;
import cn.edu.bit.newnewcc.ir.value.Constant;

/**
 * void值
 * <p>
 * ReturnInst在返回void时会用到该值
 */
public class VoidValue extends Constant {
    private VoidValue() {
        super(VoidType.getInstance());
    }

    @Override
    public String getValueName() {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean isFilledWithZero() {
        return true;
    }

    @Override
    public String getValueNameIR() {
        throw new UnsupportedOperationException();
    }

    private static class Holder {
        private static final VoidValue INSTANCE = new VoidValue();
    }

    public static VoidValue getInstance() {
        return Holder.INSTANCE;
    }
}
