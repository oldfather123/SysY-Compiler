package IR.IRInstruction;

import IR.IRValueRef.IRBaseBlockRef;
import IR.IRValueRef.IRValueRef;

import java.util.List;

import static IR.IRType.IRInt1Type.IRInt1Type;

public class ZextInstruction extends IRInstruction{
    public ZextInstruction(List<IRValueRef> operands, IRBaseBlockRef basicBlock){
        super(operands,basicBlock);
    }

    @Override
    public String toString() {
        IRValueRef resRegister = getOperands().get(0);
        IRValueRef valueRef = getOperands().get(1);
        // special val
        IRValueRef typeValRef = getOperands().get(2);
        return resRegister.getText() + " = zext " + IRInt1Type().getText() + " " + valueRef.getText() + " to " + typeValRef.getType().getText();
    }
}
