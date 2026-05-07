package backend.asm.instr.arm.binary;

import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

public class ARMFAdd extends ARMBinary {
    public ARMFAdd(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        super(operand1, operand2, parentBB, reg);
    }

    protected String printIns() {
        return "FADD " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
    }
}
