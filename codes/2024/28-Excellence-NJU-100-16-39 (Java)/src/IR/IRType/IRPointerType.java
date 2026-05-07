package IR.IRType;

import IR.IRConst;

public class IRPointerType implements IRType{
    /*
    * IRPointer可以理解为指针类型，可以表示为 BaseType + *
    * e.g. i32*, i32**就是指针类型
     */
    private IRType baseType;

    public IRPointerType(IRType baseType) {
        this.baseType = baseType;
    }

    public IRType getBaseType() {
        return baseType;
    }

    @Override
    public String getText() {
        return baseType.getText() + "*";
    }
    @Override
    public int getTypeKind() {
        return IRConst.IRConstantPointerValueKind;
    }
}
