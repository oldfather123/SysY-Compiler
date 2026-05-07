package backend.asm.instr.arm.cmp;

import backend.asm.ASMValue;
import backend.asm.instr.arm.ARMIns;
import backend.asm.instr.arm.ARMCond;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

import java.util.List;

/**
 * CSET = Conditional Set 根据某个条件是否成立，将目标寄存器设置为 1（成立）或 0（不成立）
 */
public class ARMCset extends ARMIns {
    private final ARMCond cond;

    public ARMCset(ARMCond cond, ASMBasicBlock parentBlock, Reg reg) {
        super(parentBlock, reg);
        this.cond = cond;
    }

    public ARMCond getCond() {
        return cond;
    }

    @Override
    public List<ASMValue> getOperands() {
        return null;
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        throw new RuntimeException("CSET 指令不接受操作数");
    }

    @Override
    protected String printIns() {
        return "CSET " + this.printAsOperand() + ", " + cond.toString();
    }
}
