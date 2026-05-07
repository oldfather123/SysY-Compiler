package backend.asm.instr.rv.branch;

import backend.asm.ASMValue;
import backend.asm.instr.rv.RVIns;
import backend.asm.instr.tags.BranchIns;
import backend.asm.register.store.RegStore;
import backend.asm.structure.ASMBasicBlock;

import java.util.Arrays;
import java.util.List;

public abstract class RVBranch extends RVIns implements BranchIns {
    protected ASMValue operand1;
    protected ASMValue operand2;
    private final ASMBasicBlock target;
    private final String operationName;
    
    public RVBranch(String opName, ASMValue operand1, ASMValue operand2,
                    ASMBasicBlock target, ASMBasicBlock parentBlock) {
        super(parentBlock, RegStore.NA);
        this.operationName = opName;
        this.target = target;
        this.operand1 = operand1;
        this.operand2 = operand2;
        
        addUsedVal(operand1);
        addUsedVal(operand2);
    }
    
    public abstract RVBranch createInvertedBranch(ASMBasicBlock target, ASMBasicBlock parentBlock);

    @Override
    public ASMBasicBlock getTarget() {
        return target;
    }
    
    @Override
    public List<ASMValue> getOperands() {
        return Arrays.asList(operand1, operand2);
    }
    
    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 2) {
            throw new RuntimeException("Branch 指令接受且只接受两个操作数");
        }
        
        this.modifyUse(operand1, values.get(0));
        this.modifyUse(operand2, values.get(1));
        this.operand1 = values.get(0);
        this.operand2 = values.get(1);
    }
    
    @Override
    protected String printIns() {
        return operationName + " " + operand1.printAsOperand() +
                ", " + operand2.printAsOperand() + ", " + target.printAsOperand();
    }
}
