package backend.asm.instr.arm.jump;

import backend.asm.ASMValue;
import backend.asm.instr.arm.ARMIns;
import backend.asm.instr.tags.CallIns;
import backend.asm.register.store.RegStore;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.structure.ASMFunction;

import java.util.List;

/**
 * 跳转到标签，用于函数调用，相当于 jal
 */
public class ARMBl extends ARMIns implements CallIns {
    private final ASMFunction targetFunc;

    public ARMBl(ASMFunction targetFunc, ASMBasicBlock parentBlock) {
        super(parentBlock, RegStore.NA);
        this.targetFunc = targetFunc;
    }

    @Override
    public ASMFunction getTargetFunc() {
        return targetFunc;
    }

    @Override
    public List<ASMValue> getOperands() {
        return null;
    }

    @Override
    public void resetOperands(List<ASMValue> values) {
        throw new RuntimeException("BL 指令不接受操作数");
    }

    @Override
    protected String printIns() {
        return "BL " + targetFunc.toString();
    }
}
