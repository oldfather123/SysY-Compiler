package backend.asm.instr.rv.arithmetic;

import backend.asm.immediate.ASMImmediate;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.ASMValue;

public class RVXor extends RVArithmetic {
    public RVXor(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg) {
        super(operand1, operand2, parentBB, reg);
    }

    @Override
    protected String printIns() {
        if (operand2 instanceof ASMImmediate) {
            return "xori " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
        } else {
            return "xor " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
        }
    }
}
