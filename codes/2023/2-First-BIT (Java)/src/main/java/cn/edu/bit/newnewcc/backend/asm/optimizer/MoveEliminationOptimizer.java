package cn.edu.bit.newnewcc.backend.asm.optimizer;

import cn.edu.bit.newnewcc.backend.asm.AsmFunction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmMove;

import java.util.Iterator;
import java.util.List;

public class MoveEliminationOptimizer implements Optimizer {
    @Override
    public boolean runOn(AsmFunction function) {
        List<AsmInstruction> instrList = function.getInstrList();

        int count = 0;

        Iterator<AsmInstruction> iterator = instrList.iterator();
        while (iterator.hasNext()) {
            AsmInstruction instr = iterator.next();
            if (instr instanceof AsmMove moveInstr && moveInstr.getOperand(1).equals(moveInstr.getOperand(2))) {
                iterator.remove();
                ++count;
            }
        }

        return count > 0;
    }
}
