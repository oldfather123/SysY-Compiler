package IR.IRInstruction;

import IR.IRConst;
import IR.IRValueRef.IRBaseBlockRef;
import IR.IRValueRef.IRValueRef;

import java.util.List;

public class CompareInstruction extends IRInstruction{
    int compareType;
    public CompareInstruction(List<IRValueRef> operands, IRBaseBlockRef basicBlock, int icmpType){
        super(operands, basicBlock);
        this.compareType = icmpType;
    }




    @Override
    public String toString(){
        IRValueRef resRegister = getOperands().get(0);
        IRValueRef lhs = getOperands().get(1);
        IRValueRef rhs = getOperands().get(2);
        return resRegister.getText() + " = icmp " + IRConst.compareTypes[compareType] + " "
                + lhs.getType().getText() + " " + lhs.getText() + ", " + rhs.getText();
    }

    public int getCompareType() {
        return compareType;
    }
}
