package backend.asm.instr.rv.arithmetic;

import backend.asm.immediate.ASMImmediate;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.ASMValue;

/**
 * and $t1,$t2,$t3         Bitwise AND : Set $t1 to bitwise AND of $t2 and $t3
 */
public class RVAnd extends RVArithmetic {
    public RVAnd(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg) {
        super(operand1, operand2, parentBB, reg);
    }

    @Override
    protected String printIns() {
        if (operand2 instanceof ASMImmediate) {
            return "andi " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
        } else {
            return "and " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
        }
    }
}
