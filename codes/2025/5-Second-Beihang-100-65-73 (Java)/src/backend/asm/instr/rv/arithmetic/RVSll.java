package backend.asm.instr.rv.arithmetic;

import backend.asm.ASMValue;
import backend.asm.immediate.ASMImmediate;
import backend.asm.instr.tags.ShiftIns;
import backend.asm.register.Reg;
import backend.asm.register.vir.VirIReg;
import backend.asm.structure.ASMBasicBlock;

public class RVSll extends RVArithmetic implements ShiftIns {

    public RVSll(Reg operand1, ASMValue operand2, ASMBasicBlock parentBB, Reg reg) {
        super(operand1, operand2, parentBB, reg);
    }

    @Override
    protected String printIns() {
        String op;
        if (getRegister() instanceof VirIReg virIReg && !virIReg.isDouble()) {
            if (operand2 instanceof ASMImmediate) {
                op = "slliw";
            } else {
                op = "sllw";
            }
        } else {
            if (operand2 instanceof ASMImmediate) {
                op = "slli";
            } else {
                op = "sll";
            }
        }
        return op + " " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
    }

    @Override
    public ASMImmediate getShiftImm() {
        return (ASMImmediate) this.operand2;
    }

    public ASMValue getOperand2() {
        return operand2;
    }

    public ASMValue getOperand1() {
        return operand1;
    }

    @Override
    public Type getType() {
        return Type.LOGICAL_LEFT;
    }
}
