package IR.IRInstruction;

import IR.IRValueRef.IRBaseBlockRef;
import IR.IRValueRef.IRValueRef;

import java.util.List;

public class StoreInstruction extends IRInstruction{
    /*
     * 该类是IR中的store指令
     * 用于将一个值存储到指定的地址中
     * 对应LLVM IR中的store指令
     * 指令的定义为：
     * <result> = store <ty> <value>, <ty>* <pointer>
     * 意为将value存储到pointer指向的地址中
     * 例如： store i32 %
     * tmp_1, i32* %c, align 4
     */
    public StoreInstruction(List<IRValueRef> operands, IRBaseBlockRef BaseBlock) {
        super(operands, BaseBlock);
    }
    @Override
    public String toString() {
        return "store " + getOperands().get(1).getType().getText() + " " +
                getOperands().get(1).getText() + ", " +
                getOperands().get(2).getType().getText() + " " + getOperands().get(2).getText();
    }
}
