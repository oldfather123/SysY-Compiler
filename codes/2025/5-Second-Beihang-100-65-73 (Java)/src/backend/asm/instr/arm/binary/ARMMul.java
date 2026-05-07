package backend.asm.instr.arm.binary;


import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

/**
 * 整数乘法
 */
public class ARMMul extends ARMBinary {
    public ARMMul(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        super(operand1, operand2, parentBB, reg);
    }

    @Override
    protected String printIns() {
        return "MUL " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
    }
}
