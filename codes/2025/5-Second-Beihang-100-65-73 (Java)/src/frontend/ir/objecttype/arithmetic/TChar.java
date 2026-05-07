package frontend.ir.objecttype.arithmetic;

import frontend.ir.Value;
import frontend.ir.constant.CharConst;

import java.util.List;

public class TChar extends ArithmeticType {
    @Override
    public String printIRType() {
        return "i8";
    }

    @Override
    public boolean equals(Object obj) {
        return obj instanceof TChar;
    }

    @Override
    public Value getDefaultValue() {
        return CharConst.Zero;
    }

    @Override
    public List<Integer> getSizeList() {
        throw new RuntimeException("Char类型不会出现在数组中");
    }

    public int getByteSize() {
        return 1; // i8类型占用1个字节
    }
}
