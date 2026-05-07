package IR.IRInstruction;

import IR.IRValueRef.IRBaseBlockRef;
import IR.IRValueRef.IRValueRef;

import java.util.List;

public class ReturnInstruction extends IRInstruction{
    public ReturnInstruction(List<IRValueRef> operands, IRBaseBlockRef baseBlock){
        super(operands,baseBlock);
    }
    @Override
    public String toString() {
        if (getOperands().get(0) == null) {
            return "ret";
        }
        return "ret " + getOperands().get(0).getType().getText() + " " + getOperands().get(0).getText();
    }
}
