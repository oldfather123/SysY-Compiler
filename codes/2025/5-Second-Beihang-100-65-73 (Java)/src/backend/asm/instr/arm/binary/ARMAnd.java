package backend.asm.instr.arm.binary;

import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

/**
 * 按位与
 */
public class ARMAnd extends ARMBinary {
    public ARMAnd(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        super(operand1, operand2, parentBB, reg);
    }

    @Override
    protected String printIns() {
        return "AND " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
    }
}
