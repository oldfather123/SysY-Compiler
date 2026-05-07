package IR.IRInstruction;

import IR.IRType.IRType;
import IR.IRValueRef.IRBaseBlockRef;
import IR.IRValueRef.IRValueRef;

import java.util.List;

public class AllocateInstruction extends IRInstruction{
    /*
     * 该类是IR中的分配指令，用于分配内存
     * 对应代码中的alloca
     * 例子：%c = alloca i32, align 4
     */
    public AllocateInstruction(List<IRValueRef> operands, IRBaseBlockRef BaseBlock) {
        super(operands, BaseBlock);
    }

    @Override
    public String toString() {
        IRValueRef resRegister = getOperands().get(0);
        return resRegister.getText() + " = alloca " + resRegister.getType().getText();
    }

    public String getName(){
        return getOperands().get(0).getText();
    }

    public IRType getType(){
        return getOperands().get(0).getType();
    }
}
