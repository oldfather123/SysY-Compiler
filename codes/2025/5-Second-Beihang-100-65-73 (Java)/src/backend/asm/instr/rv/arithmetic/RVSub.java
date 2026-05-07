package backend.asm.instr.rv.arithmetic;

import backend.asm.register.Reg;
import backend.asm.immediate.ASMImmediate;
import backend.asm.register.vir.VirIReg;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.ASMValue;

/**
 * sub $t1,$t2,$t3         Subtraction with overflow : set $t1 to ($t2 minus $t3)
 */
public class RVSub extends RVArithmetic {

    public RVSub(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg) {
        super(operand1, operand2, parentBB, reg);
    }


    @Override
    protected String printIns() {
        if (getRegister() instanceof VirIReg virIReg && !virIReg.isDouble()) {
            if (operand2 instanceof ASMImmediate) {
                return "addiw " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", -" + operand2.printAsOperand();
            } else {
                return "subw " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
            }
        } else {
            if (operand2 instanceof ASMImmediate) {
                return "addi " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", -" + operand2.printAsOperand();
            } else {
                return "sub " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
            }
        }
    }
}
