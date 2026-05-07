package frontend.ir.objecttype.arithmetic;

import frontend.ir.Value;
import frontend.ir.constant.BoolConst;

import java.util.List;

public class TBool extends ArithmeticType {
    @Override
    public String printIRType() {
        return "i1";
    }

    @Override
    public boolean equals(Object obj) {
        return obj instanceof TBool;
    }

    @Override
    public Value getDefaultValue() {
        return BoolConst.Zero;
    }

    @Override
    public List<Integer> getSizeList() {
        throw new RuntimeException("Bool类型不会出现在数组中");
    }

    public int getByteSize() {
        return 1; // i1类型占用1个字节
    }
}
