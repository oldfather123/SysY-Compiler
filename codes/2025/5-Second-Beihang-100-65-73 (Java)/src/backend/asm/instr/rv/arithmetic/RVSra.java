package backend.asm.instr.rv.arithmetic;

import backend.asm.instr.tags.ShiftIns;
import backend.asm.register.Reg;
import backend.asm.immediate.ASMImmediate;
import backend.asm.register.vir.VirIReg;
import backend.asm.structure.ASMBasicBlock;

public class RVSra extends RVArithmetic implements ShiftIns {

    public RVSra(Reg operand1, ASMImmediate operand2, ASMBasicBlock parentBlock, Reg reg) {
        super(operand1, operand2, parentBlock, reg);
    }

    @Override
    protected String printIns() {
        String op;
        if (getRegister() instanceof VirIReg virIReg && !virIReg.isDouble()) {
            if (operand2 instanceof ASMImmediate) {
                op = "sraiw";
            } else {
                op = "sraw";
            }
        } else {
            if (operand2 instanceof ASMImmediate) {
                op = "srai";
            } else {
                op = "sra";
            }
        }
        return op + " " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
    }

    @Override
    public ASMImmediate getShiftImm() {
        return (ASMImmediate) this.operand2;
    }

    @Override
    public Type getType() {
        return Type.ARITHMETIC_RIGHT;
    }
}
