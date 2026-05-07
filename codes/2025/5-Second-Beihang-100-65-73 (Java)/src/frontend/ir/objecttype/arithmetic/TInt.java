package frontend.ir.objecttype.arithmetic;

import frontend.ir.Value;
import frontend.ir.constant.IntConst;

import java.util.List;

public class TInt extends ArithmeticType {
    @Override
    public String printIRType() {
        return "i32";
    }

    @Override
    public boolean equals(Object obj) {
        return obj instanceof TInt;
    }

    @Override
    public Value getDefaultValue() {
        return IntConst.Zero;
    }

    @Override
    public List<Integer> getSizeList() {
        return List.of(4);
    }

    public int getByteSize() {
        return 4; // i32类型占用4个字节
    }
}
