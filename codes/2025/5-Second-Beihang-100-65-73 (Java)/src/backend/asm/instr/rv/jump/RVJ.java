package backend.asm.instr.rv.jump;

import backend.asm.ASMValue;
import backend.asm.instr.rv.RVIns;
import backend.asm.instr.tags.JumpIns;
import backend.asm.register.store.RegStore;
import backend.asm.structure.ASMBasicBlock;

import java.util.List;

/**
 * j target                Jump unconditionally : Jump to statement at target address
 */
public class RVJ extends RVIns implements JumpIns {
    private final ASMBasicBlock target;
    
    public RVJ(ASMBasicBlock target, ASMBasicBlock parentBlock) {
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
        throw new RuntimeException("J 指令不接受操作数");
    }
    
    @Override
    protected String printIns() {
        return "j " + target.printAsOperand();
    }
}
