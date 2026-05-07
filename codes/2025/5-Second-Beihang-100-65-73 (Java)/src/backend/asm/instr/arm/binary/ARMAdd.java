package backend.asm.instr.arm.binary;

import backend.asm.ASMValue;
import backend.asm.instr.tags.AddIns;
import backend.asm.register.Reg;
import backend.asm.register.store.ArmRegStore;
import backend.asm.register.vir.VirIReg;
import backend.asm.structure.ASMBasicBlock;

/**
 * 整数加法
 */
public class ARMAdd extends ARMBinary implements AddIns {
    public ARMAdd(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg) {
        super(operand1, operand2, parentBB, reg);
    }
    
    @Override
    public ASMValue getOperand1() {
        return operand1;
    }
    
    @Override
    public ASMValue getOperand2() {
        return operand2;
    }
    
    @Override
    protected String printIns() {
        if (operand1 instanceof VirIReg virIReg1 && operand2 instanceof VirIReg virIReg2 &&
                virIReg1.isDouble() && !virIReg2.isDouble()) {
            return "ADD " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand() + ", SXTW";
        } else if (operand1 == ArmRegStore.getInstance().getStackPtr()
                && operand2 instanceof VirIReg virIReg2 && !virIReg2.isDouble()) {
            return "ADD " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand() + ", SXTW";
        }
        return "ADD " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
    }
}
