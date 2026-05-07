package backend.asm.instr.rv.branch;

import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

/**
 * beq $t1,$t2,label       Branch if equal : Branch to statement at label's address if $t1 and $t2 are equal
 */
public class RVBeq extends RVBranch {
    public RVBeq(Reg operand1, Reg operand2, ASMBasicBlock target, ASMBasicBlock parentBlock) {
        super("beq", operand1, operand2, target, parentBlock);
    }
    
    @Override
    public RVBranch createInvertedBranch(ASMBasicBlock target, ASMBasicBlock parentBlock) {
        return new RVBne((Reg) this.operand1, (Reg) this.operand2, target, parentBlock);
    }
}
