package backend.asm.instr.arm.binary;

import backend.asm.immediate.ASMImmediate;
import backend.asm.instr.tags.ShiftIns;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

/**
 * 算术右移（移动量为立即数）
 */
public class ARMAsr extends ARMBinary implements ShiftIns {
    public ARMAsr(Reg operand1, ASMImmediate operand2, ASMBasicBlock parentBlock, Reg reg) {
        super(operand1, operand2, parentBlock, reg);
    }

    @Override
    protected String printIns() {
        return "ASR " + this.printAsOperand() + ", " + operand1.printAsOperand() + ", " + operand2.printAsOperand();
    }

    @Override
    public ASMImmediate getShiftImm() {
        return (ASMImmediate) this.operand2;
    }

    @Override
    public ShiftIns.Type getType() {
        return ShiftIns.Type.ARITHMETIC_RIGHT;
    }
}
