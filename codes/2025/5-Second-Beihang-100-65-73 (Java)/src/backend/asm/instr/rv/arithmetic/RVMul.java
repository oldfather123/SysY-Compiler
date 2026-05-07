package backend.asm.instr.rv.arithmetic;

import backend.asm.register.Reg;
import backend.asm.immediate.ASMImmediate;
import backend.asm.register.store.RegStore;
import backend.asm.register.vir.VirIReg;
import backend.asm.structure.ASMBasicBlock;

/**
 * mul $t1,$t2,$t3         Multiplication without overflow  : Set HI to high-order 32 bits, LO and $t1 to low-order 32 bits of the product of $t2 and $t3 (use mfhi to access HI, mflo to access LO)
 */
public class RVMul extends RVArithmetic {
    public RVMul(Reg operand1, Reg operand2, ASMBasicBlock parentBB, Reg reg) {
        super(operand1, operand2, parentBB, reg);
    }

    @Override
    protected String printIns() {
        String op;
        if (getRegister() instanceof VirIReg virIReg && !virIReg.isDouble()) {
            op = "mulw";
        } else {
            op = "mul";
        }
        return op + " " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
    }
}
