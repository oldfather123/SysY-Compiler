package backend.asm.instr.rv.others;

import backend.asm.ASMValue;
import backend.asm.instr.rv.RVIns;
import backend.asm.register.store.RegStore;
import backend.asm.structure.ASMBasicBlock;

import java.util.List;

public class RVSyscall extends RVIns {
    public RVSyscall(ASMBasicBlock parentBlock) {
        super(parentBlock, RegStore.NA);
    }
    
    @Override
    public List<ASMValue> getOperands() {
        return null;
    }
    
    @Override
    public void resetOperands(List<ASMValue> values) {
        throw new RuntimeException("SYSCALL 指令不接受操作数");
    }
    
    @Override
    protected String printIns() {
        return "syscall";
    }
}
