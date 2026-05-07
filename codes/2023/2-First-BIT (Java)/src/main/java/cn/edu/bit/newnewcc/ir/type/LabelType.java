package cn.edu.bit.newnewcc.ir.type;

import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.value.Constant;

/**
 * Label类型
 * <p>
 * 该类型是单例
 */
public class LabelType extends Type {
    private LabelType() {
    }

    @Override
    protected String getTypeName_() {
        return "label";
    }

    @Override
    public Constant getZeroInitialization() {
        throw new UnsupportedOperationException();
    }

    @Override
    public long getSize() {
        // 理论上应该返回指针的位数，但是SysY不涉及指针，所以这个方法应该不会被调用到
        throw new UnsupportedOperationException();
    }

    private static class Holder {
        private static final LabelType INSTANCE = new LabelType();
    }

    /**
     * 获取Label类型的实例
     *
     * @return Label类型的唯一实例
     */
    public static LabelType getInstance() {
        return Holder.INSTANCE;
    }
}
