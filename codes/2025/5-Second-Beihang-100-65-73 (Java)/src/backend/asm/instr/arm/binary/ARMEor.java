package backend.asm.instr.arm.binary;

import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

/**
 * 按位异或
 */
public class ARMEor extends ARMBinary {
    public ARMEor(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        super(operand1, operand2, parentBB, reg);
    }

    @Override
    protected String printIns() {
        return "EOR " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
    }
}
