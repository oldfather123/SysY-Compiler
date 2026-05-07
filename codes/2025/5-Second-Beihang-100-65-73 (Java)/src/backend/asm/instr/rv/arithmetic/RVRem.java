package backend.asm.instr.rv.arithmetic;

import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

public class RVRem extends RVArithmetic {
    public RVRem(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        super(operand1, operand2, parentBB, reg);
    }

    @Override
    protected String printIns() {
        return "remw " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
    }
}
