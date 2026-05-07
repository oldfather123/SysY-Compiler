package IR.IRType;

import IR.IRConst;

public class IRInt1Type implements IRType{
    private static final String name = "i1";
    private static IRInt1Type int1Type = null;/*将初始化延迟到第一次使用，同Int32，保证全局只有一个INT1对象*/

    private IRInt1Type() {}

    public static IRType IRInt1Type() {
        if (int1Type == null) {
            int1Type = new IRInt1Type();
        }
        return int1Type;
    }

    @Override
    public String getText() {
        return name;
    }
    @Override
    public int getTypeKind() {
        return IRConst.IRConstantInt1ValueKind;
    }
}
