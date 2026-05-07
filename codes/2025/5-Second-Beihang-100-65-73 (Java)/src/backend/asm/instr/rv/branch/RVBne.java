package backend.asm.instr.rv.branch;

import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

public class RVBne extends RVBranch {
    public RVBne(Reg operand1, Reg operand2, ASMBasicBlock target, ASMBasicBlock parentBlock) {
        super("bne", operand1, operand2, target, parentBlock);
    }
    
    @Override
    public RVBranch createInvertedBranch(ASMBasicBlock target, ASMBasicBlock parentBlock) {
        return new RVBeq((Reg) this.operand1, (Reg) this.operand2, target, parentBlock);
    }
}
