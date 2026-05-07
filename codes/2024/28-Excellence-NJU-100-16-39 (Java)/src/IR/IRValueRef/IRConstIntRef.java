package IR.IRValueRef;

import IR.IRConst;
import IR.IRType.IRInt32Type;
import IR.IRType.IRType;

public class IRConstIntRef implements IRValueRef{
    /*
     * 该类是整数常量的引用，用于表示整数常量
     * 例如 1
     */
    private int value;
    private IRType type;
    public IRConstIntRef(int value, IRType type){
        this.value = value;
        this.type = type;
    }
    public int getValue() {
        return value;
    }
    @Override
    public  String getText(){
        return "" + value;
    }
    @Override
    public IRType getType() {
        return type;
    }
    @Override
    public int getTypeKind() {
        if(type.equals(IRInt32Type.IRInt32Type())){
            return IRConst.IRConstantInt32ValueKind;
        }else{
            return IRConst.IRConstantInt1ValueKind;
        }
    }
}
