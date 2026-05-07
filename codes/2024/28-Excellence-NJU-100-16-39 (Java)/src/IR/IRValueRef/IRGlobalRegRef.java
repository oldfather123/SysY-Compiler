package IR.IRValueRef;

import IR.IRConst;
import IR.IRType.IRType;

public class IRGlobalRegRef implements IRValueRef{
    /*
     * 该类是全局寄存器的引用，用于表示全局寄存器
     * 例如 @.str
     */
    static int globalCounter = 0;
    private final IRType type;
    private final int globalNO;
    private String identity;

    public IRGlobalRegRef(String identity, IRType type) {
        this.identity = identity;
        this.globalNO = globalCounter++;
        this.type = type;
    }
    @Override
    public String getText() {
        return "@" + this.identity + globalNO;
    }
    @Override
    public IRType getType() {
        return type;
    }
    @Override
    public int getTypeKind() {
        //todo：这里返回的是全局变量的类型
        return IRConst.IRGlobalVariableValueKind;
    }
    public String getIdentity() {
        return identity;
    }
}
