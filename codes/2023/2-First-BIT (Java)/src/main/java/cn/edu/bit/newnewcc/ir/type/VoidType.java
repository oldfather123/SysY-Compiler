package cn.edu.bit.newnewcc.ir.type;

import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.value.Constant;

/**
 * Void类型
 * <p>
 * 该类型是单例
 */
public class VoidType extends Type {
    private VoidType() {
    }

    @Override
    protected String getTypeName_() {
        return "void";
    }

    @Override
    public Constant getZeroInitialization() {
        throw new UnsupportedOperationException();
    }

    @Override
    public long getSize() {
        throw new UnsupportedOperationException();
    }

    private static class Holder {
        private static final VoidType INSTANCE = new VoidType();
    }

    /**
     * 获取Void类型的实例
     *
     * @return Void类型的唯一实例
     */
    public static VoidType getInstance() {
        return Holder.INSTANCE;
    }
}
