package backend.asm.instr.rv.jump;

import backend.asm.instr.rv.RVIns;
import backend.asm.register.Reg;
import backend.asm.register.store.RegStore;
import backend.asm.structure.ASMBasicBlock;
import backend.asm.ASMValue;

import java.util.Collections;
import java.util.List;

/**
 * jr $t1                  Jump register unconditionally : Jump to statement whose address is in $t1
 */
public class RVJr extends RVIns {
    private Reg target;
    
    public RVJr(Reg target, ASMBasicBlock parentBlock) {
        super(parentBlock, RegStore.NA);
        this.target = target;
        
        this.addUsedVal(target);
    }
    
    @Override
    public List<ASMValue> getOperands() {
        return Collections.singletonList(target);
    }
    
    @Override
    public void resetOperands(List<ASMValue> values) {
        if (values == null || values.size() != 1) {
            throw new RuntimeException("JR 指令接受且只接受一个操作数");
        }
        
        this.modifyUse(target, values.getFirst());
        this.target = (Reg) values.getFirst();
    }
    
    @Override
    protected String printIns() {
        return "jr " + target.printAsOperand();
    }
}
