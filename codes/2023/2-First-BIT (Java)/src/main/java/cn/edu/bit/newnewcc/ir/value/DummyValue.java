package cn.edu.bit.newnewcc.ir.value;

import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.type.DummyType;

public class DummyValue extends Value {
    public DummyValue() {
        super(DummyType.getInstance());
    }

    @Override
    public String getValueName() {
        throw new UnsupportedOperationException();
    }

    @Override
    public String getValueNameIR() {
        throw new UnsupportedOperationException();
    }

    @Override
    public void setValueName(String valueName) {
        throw new UnsupportedOperationException();
    }
}
