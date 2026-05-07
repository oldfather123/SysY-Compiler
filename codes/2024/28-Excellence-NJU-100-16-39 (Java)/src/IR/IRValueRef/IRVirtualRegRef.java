package IR.IRValueRef;

import IR.IRConst;
import IR.IRType.IRType;

public class IRVirtualRegRef implements IRValueRef{
    /*
     * 该类是虚拟寄存器的引用，用于表示虚拟寄存器
     * 例如 %tmp1
     * 一般会有alloca指令为其分配空间
     */
    static int tempCounter = 0;
    private final String identity;/*虚拟寄存器的标识符*/
    private IRType type;/*虚拟寄存器所对应变量的类型*/
    private final int tempNO;
    public IRVirtualRegRef(String identity, IRType type) {
        this.tempNO = tempCounter++;
        this.type = type;
        this.identity = identity;
    }
    @Override
    public String getText() {
        return "%" + this.identity + tempNO;
    }
    @Override
    public IRType getType() {
        return type;
    }
    @Override
    public int getTypeKind() {
        //todo：这里返回的是一条指令的类型，比如对应的alloca指令
        return IRConst.IRInstructionValueKind;
    }
    public String getIdentity(){return identity;}
}
