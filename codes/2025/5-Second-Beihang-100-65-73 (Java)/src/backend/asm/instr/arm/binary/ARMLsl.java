package backend.asm.instr.arm.binary;

import backend.asm.immediate.ASMImmediate;
import backend.asm.instr.tags.ShiftIns;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

/**
 * 逻辑左移（移动量为立即数）
 */
public class ARMLsl extends ARMBinary implements ShiftIns {
    public ARMLsl(Reg operand1, ASMImmediate operand2, ASMBasicBlock parentBlock, Reg reg) {
        super(operand1, operand2, parentBlock, reg);
    }

    @Override
    protected String printIns() {
        return "LSL " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
    }

    @Override
    public ASMImmediate getShiftImm() {
        return (ASMImmediate) this.operand2;
    }

    @Override
    public Type getType() {
        return Type.LOGICAL_LEFT;
    }
}
