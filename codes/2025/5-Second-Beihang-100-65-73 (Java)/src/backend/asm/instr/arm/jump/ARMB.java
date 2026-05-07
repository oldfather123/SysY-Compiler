package backend.asm.instr.arm.jump;

import backend.asm.ASMValue;
import backend.asm.instr.arm.ARMIns;
import backend.asm.instr.tags.JumpIns;
import backend.asm.register.store.RegStore;
import backend.asm.structure.ASMBasicBlock;

import java.util.List;

public class ARMB extends ARMIns implements JumpIns {
    private final ASMBasicBlock target;

    public ARMB(ASMBasicBlock target, ASMBasicBlock parentBlock) {
        super(parentBlock, RegStore.NA);
        this.target = target;
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
        throw new RuntimeException("B 指令不接受操作数");
    }

    @Override
    protected String printIns() {
        return "B " + target.printAsOperand();
    }
}
