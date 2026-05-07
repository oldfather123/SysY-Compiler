package backend.asm.instr.arm.ternary;

import backend.asm.register.Reg;
import backend.asm.register.vir.VirIReg;
import backend.asm.structure.ASMBasicBlock;

public class ARMMadd extends ARMTernary {
    public ARMMadd(Reg operand1, Reg operand2, Reg operand3, ASMBasicBlock parentBB, Reg reg) {
        super(operand1, operand2, operand3, parentBB, reg);
    }

    @Override
    protected String printIns() {
        if (operand3 instanceof VirIReg virIReg && virIReg.isDouble()) {
            return "SMADDL " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", "
                    + operand2.printAsOperand() + ", " + operand3.printAsOperand();
        }
        return "MADD " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", "
                + operand2.printAsOperand() + ", " + operand3.printAsOperand();
    }

}
