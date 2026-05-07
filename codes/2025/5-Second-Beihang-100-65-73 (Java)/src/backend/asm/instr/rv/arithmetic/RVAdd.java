package backend.asm.instr.rv.arithmetic;

import backend.asm.immediate.ASMImmediate;
import backend.asm.instr.tags.AddIns;
import backend.asm.register.Reg;
import backend.asm.register.vir.VirIReg;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.ASMValue;

/**
 * add $t1,$t2,$t3         Addition with overflow : set $t1 to ($t2 plus $t3)
 */
public class RVAdd extends RVArithmetic implements AddIns {

    public RVAdd(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg) {
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
        String op;
        if (getRegister() instanceof VirIReg virIReg && !virIReg.isDouble()) {
            if (operand2 instanceof ASMImmediate) {
                op = "addiw";
            } else {
                op = "addw";
            }
        } else {
            if (operand2 instanceof ASMImmediate) {
                op = "addi";
            } else {
                op = "add";
            }
        }
        return op + " " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
    }
}
