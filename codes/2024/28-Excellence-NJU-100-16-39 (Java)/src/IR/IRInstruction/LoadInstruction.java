package IR.IRInstruction;

import IR.IRValueRef.IRBaseBlockRef;
import IR.IRValueRef.IRValueRef;

import java.util.List;


public class LoadInstruction extends IRInstruction{
    /*
    * 该类是IR中的加载指令，用于加载内存
    * 对应代码中的load
    * 例子：%b = load i32, i32* %a
     */
    public LoadInstruction(List<IRValueRef> operands, IRBaseBlockRef BaseBlock) {
        super(operands, BaseBlock);
    }
    @Override
    public String toString() {
        IRValueRef resRegister = getOperands().get(0);
        IRValueRef pointer = getOperands().get(1);
        return resRegister.getText() + " = load " + resRegister.getType().getText() + ", "
                + pointer.getType().getText() + " " + pointer.getText();
    }
}
