package cn.edu.bit.newnewcc.backend.asm.optimizer;

import cn.edu.bit.newnewcc.backend.asm.AsmFunction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmLabel;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmLoad;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmStore;
import cn.edu.bit.newnewcc.backend.asm.operand.MemoryAddress;

import java.util.Iterator;

public class SLWithSameAddress implements Optimizer {
    @Override
    public boolean runOn(AsmFunction function) {
        boolean removed = false;
        Iterator<AsmInstruction> iterator = function.getInstrList().listIterator();
        AsmInstruction last = null;
        while (iterator.hasNext()) {
            AsmInstruction now = iterator.next();
            if (last instanceof AsmStore asmStore && now instanceof AsmLoad asmLoad) {
                if (asmStore.getOperand(1).equals(asmLoad.getOperand(1))) {
                    if (asmStore.getOperand(2) instanceof MemoryAddress a1 && asmLoad.getOperand(2) instanceof MemoryAddress a2) {
                        int l1 = asmStore.getOpcode().equals(AsmStore.Opcode.SW) ? 32 : 64;
                        int l2 = asmLoad.getOpcode().equals(AsmLoad.Opcode.LW) ? 32 : 64;
                        if (a1.equals(a2) && l1 == l2) {
                            iterator.remove();
                            removed = true;
                        }
                    }
                }
            }
            last = now;
        }
        return removed;
    }
}
