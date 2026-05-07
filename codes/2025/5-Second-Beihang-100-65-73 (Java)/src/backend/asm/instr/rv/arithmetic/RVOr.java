package backend.asm.instr.rv.arithmetic;

import backend.asm.ASMValue;
import backend.asm.immediate.ASMImmediate;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

/**
 * and $t1,$t2,$t3         Bitwise AND : Set $t1 to bitwise AND of $t2 and $t3
 */
public class RVOr extends RVArithmetic {
    public RVOr(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg) {
        super(operand1, operand2, parentBB, reg);
    }

    @Override
    protected String printIns() {
        if (operand2 instanceof ASMImmediate) {
            return "ori " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
        } else {
            return "or " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
        }
    }
}
