package backend.asm.instr.arm.jump;

import backend.asm.ASMValue;
import backend.asm.instr.arm.ARMCond;
import backend.asm.instr.arm.ARMIns;
import backend.asm.instr.tags.BranchIns;
import backend.asm.register.store.RegStore;
import backend.asm.structure.ASMBasicBlock;

import java.util.List;

/**
 * 有条件跳转指令，根据当前条件位进行跳转
 */
public class ARMBranch extends ARMIns implements BranchIns {
    private final ASMBasicBlock target;
    private ARMCond cond;

    public ARMBranch(ASMBasicBlock target, ASMBasicBlock parentBlock) {
        super(parentBlock, RegStore.NA);
        this.target = target;
        this.cond = ARMCond.EQ; // 生成的时候默认是 BEQ
    }

    public void setCond(ARMCond cond) {
        this.cond = cond;
    }

    public ARMCond getCond() {
        return cond;
    }

    @Override
    public ASMBasicBlock getTarget() {
        return target;
    }

    @Override
    public List<ASMValue> getOperands() {
        return null;
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        throw new RuntimeException("BEQ 指令不接受操作数");
    }

    @Override
    protected String printIns() {
        return  "B." + cond.toString() + " " + target.printAsOperand();
    }
}
