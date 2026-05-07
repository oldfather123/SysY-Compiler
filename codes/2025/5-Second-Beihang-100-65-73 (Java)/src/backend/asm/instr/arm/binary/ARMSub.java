package backend.asm.instr.arm.binary;


import backend.asm.ASMValue;
import backend.asm.register.Reg;
import backend.asm.register.vir.VirIReg;
import backend.asm.structure.ASMBasicBlock;

/**
 * 整数减法
 */
public class ARMSub extends ARMBinary {
    public ARMSub(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg) {
        super(operand1, operand2, parentBB, reg);
    }

    @Override
    protected String printIns() {
        if (operand1 instanceof VirIReg virIReg1 && operand2 instanceof VirIReg virIReg2 &&
                virIReg1.isDouble() && !virIReg2.isDouble()) {
            return "SUB " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand() + ", SXTW";
        }
        return "SUB " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
    }
}
