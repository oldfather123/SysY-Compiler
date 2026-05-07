package cn.edu.bit.newnewcc.ir.type;

import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.value.Constant;

public class DummyType extends Type {

    private DummyType() {
    }

    @Override
    protected String getTypeName_() {
        throw new UnsupportedOperationException();
    }

    @Override
    public Constant getZeroInitialization() {
        throw new UnsupportedOperationException();
    }

    @Override
    public long getSize() {
        return 0;
    }

    private static class Holder {
        private static final DummyType INSTANCE = new DummyType();
    }

    public static DummyType getInstance() {
        return Holder.INSTANCE;
    }
}
