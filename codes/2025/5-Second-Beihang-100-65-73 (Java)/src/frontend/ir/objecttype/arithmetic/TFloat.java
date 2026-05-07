package frontend.ir.objecttype.arithmetic;

import frontend.ir.Value;
import frontend.ir.constant.FloatConst;

import java.util.List;

public class TFloat extends ArithmeticType {
    @Override
    public String printIRType() {
        return "float";
    }

    @Override
    public boolean equals(Object obj) {
        return obj instanceof TFloat;
    }

    @Override
    public Value getDefaultValue() {
        return FloatConst.Zero;
    }

    @Override
    public List<Integer> getSizeList() {
        return List.of(4);
    }

    public int getByteSize() {
        return 4; // float类型占用4个字节
    }
}
