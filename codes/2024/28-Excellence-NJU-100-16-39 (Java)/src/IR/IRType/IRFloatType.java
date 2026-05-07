package IR.IRType;

import IR.IRConst;

public class IRFloatType implements IRType{
    private static final String name = "float";
    private static IRFloatType floatType = null;

    public IRFloatType() {}

    public static IRType IRFloatType() {
        if (floatType == null) {
            floatType = new IRFloatType();
        }
        return floatType;
    }

    @Override
    public String getText() {
        return name;
    }
    @Override
    public int getTypeKind() {
        return IRConst.IRConstantFloatValueKind;
    }
}
