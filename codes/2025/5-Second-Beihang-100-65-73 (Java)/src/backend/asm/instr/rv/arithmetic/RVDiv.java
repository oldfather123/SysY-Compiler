package backend.asm.instr.rv.arithmetic;

import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

/**
 * div $t1,$t2             Division with overflow : Divide $t1 by $t2 then set LO to quotient and HI to remainder (use mfhi to access HI, mflo to access LO)
 */
public class RVDiv extends RVArithmetic {
    public RVDiv(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        super(operand1, operand2, parentBB, reg);
    }

    @Override
    protected String printIns() {
        return "divw " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
    }
}
