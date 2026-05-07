package IR.IRType;

import IR.IRConst;

public class IRVoidType implements IRType{
    private static final String name = "void";
    private static IRVoidType voidType = null;

    private IRVoidType() {}

    public static IRType IRVoidType() {
        if (voidType == null) {
            voidType = new IRVoidType();
        }
        return voidType;
    }

    @Override
    public String getText() {
        return name;
    }
    @Override
    public int getTypeKind() {
        return IRConst.IRConstantVoidValueKind;
    }
}
