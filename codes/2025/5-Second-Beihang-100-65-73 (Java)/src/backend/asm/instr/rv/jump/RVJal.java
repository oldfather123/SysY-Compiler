package backend.asm.instr.rv.jump;

import backend.asm.ASMValue;
import backend.asm.instr.rv.RVIns;
import backend.asm.instr.tags.CallIns;
import backend.asm.register.store.RegStore;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.structure.ASMFunction;

import java.util.List;

/**
 * jal target              Jump and link : Set $ra to Program Counter (return address) then jump to statement at target address
 */
public class RVJal extends RVIns implements CallIns {
    private final ASMFunction targetFunc;
    
    public RVJal(ASMFunction targetFunc, ASMBasicBlock parentBlock) {
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
        throw new RuntimeException("JAL 指令不接受操作数");
    }
    
    @Override
    protected String printIns() {
        return "jal " + targetFunc.toString();
    }
}
