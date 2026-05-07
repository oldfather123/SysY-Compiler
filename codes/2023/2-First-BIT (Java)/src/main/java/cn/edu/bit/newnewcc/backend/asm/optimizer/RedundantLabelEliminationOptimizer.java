package cn.edu.bit.newnewcc.backend.asm.optimizer;

import cn.edu.bit.newnewcc.backend.asm.AsmFunction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmJump;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmLabel;
import cn.edu.bit.newnewcc.backend.asm.operand.Label;

import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

public class RedundantLabelEliminationOptimizer implements Optimizer {
    @Override
    public boolean runOn(AsmFunction function) {
        List<AsmInstruction> instrList = function.getInstrList();

        Set<Label> usefulLabels = new HashSet<>();
        for (AsmInstruction instr : instrList) {
            if (instr instanceof AsmJump) {
                for (int i = 1; i <= 3; ++i) {
                    if (instr.getOperand(i) instanceof Label label) {
                        usefulLabels.add(label);
                    }
                }
            }
        }

        int count = 0;

        Iterator<AsmInstruction> iterator = instrList.iterator();
        while (iterator.hasNext()) {
            AsmInstruction instr = iterator.next();
            if (instr instanceof AsmLabel labelInstr && !usefulLabels.contains(labelInstr.getLabel())) {
                iterator.remove();
                ++count;
            }
        }

        return count > 0;
    }
}
