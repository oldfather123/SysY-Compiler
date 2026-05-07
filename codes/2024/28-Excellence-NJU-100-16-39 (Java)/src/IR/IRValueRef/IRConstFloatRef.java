package IR.IRValueRef;

import IR.IRConst;
import IR.IRType.IRFloatType;
import IR.IRType.IRType;


public class IRConstFloatRef implements IRValueRef{
    /*
     * 该类是浮点数常量的引用，用于表示浮点数常量
     * 例如 1.0
     */
    private final float value;
    private final IRType type;
    private final String hexNumber;
    public IRConstFloatRef(float value) {
        this.value = value;
        this.type = IRFloatType.IRFloatType();
        //测试用
        this.hexNumber = "0x" + Long.toHexString(Double.doubleToLongBits(value));
    }
    public float getValue() {
        return value;
    }
    @Override
    public String getText() {
        return hexNumber;
    }
    @Override
    public IRType getType() {
        return type;
    }
    @Override
    public int getTypeKind() {
        return IRConst.IRConstantFloatValueKind;
    }
    public String getHexNumber() {
        return hexNumber;
    }
}
