package backend.asm.instr.arm.ternary;

import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

/**
 * 先乘后减 MSUB  `<Xd>, <Xn>, <Xm>, <Xa>`  <==>  `Xd = Xa - (Xn * Xm)`
 */
public class ARMMsub extends ARMTernary {
    public ARMMsub(Reg operand1, Reg operand2, Reg operand3, ASMBasicBlock parentBB, Reg reg) {
        super(operand1, operand2, operand3, parentBB, reg);
    }

    @Override
    protected String printIns() {
        return "MSUB " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", "
                + operand2.printAsOperand() + ", " + operand3.printAsOperand();
    }
}
