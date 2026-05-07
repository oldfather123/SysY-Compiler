package IR.IRInstruction;

import IR.IRValueRef.IRBaseBlockRef;
import IR.IRValueRef.IRValueRef;

import java.util.List;

public class BranchInstruction extends IRInstruction{
    public final static int SINGLE = 0;
    public final static int DOUBLE = 1;
    private int type;
    private IRBaseBlockRef baseBlock1;
    private IRBaseBlockRef baseBlock2;
    public BranchInstruction(List<IRValueRef> operands, IRBaseBlockRef basicBlock) {
        super(operands, basicBlock);
        if(operands.size()==1) {
            this.baseBlock1 = (IRBaseBlockRef) operands.get(0);
            this.type = SINGLE;
        }else{
            this.baseBlock1 = (IRBaseBlockRef) operands.get(1);
            this.baseBlock2 = (IRBaseBlockRef) operands.get(2);
            this.type = DOUBLE;
        }
    }

    public IRBaseBlockRef getBaseBlock1() {
        return baseBlock1;
    }

    public IRBaseBlockRef getBaseBlock2() {
        return baseBlock2;
    }

    public int getType(){
        return this.type;
    }
    @Override
    public String toString() {
        if(type == SINGLE){//单一跳转
            return "br label " + baseBlock1.getLabel();
        }else{//选择跳转
            IRValueRef register = getOperands().get(0);
            return "br i1 "+ register.getText() + ", label " +baseBlock1.getLabel() + ", label " + baseBlock2.getLabel();
        }
    }
}
