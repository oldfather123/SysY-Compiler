package backend.asm.instr.rv.arithmetic;

import backend.asm.ASMValue;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

public class RVFDiv extends RVArithmetic {

    public RVFDiv(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg) {
        super(operand1, operand2, parentBB, reg);
    }

    @Override
    protected String printIns() {
        return "fdiv.s " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
    }
}
