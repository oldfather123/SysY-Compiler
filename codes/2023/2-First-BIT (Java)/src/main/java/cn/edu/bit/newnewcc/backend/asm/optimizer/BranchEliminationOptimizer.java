package cn.edu.bit.newnewcc.backend.asm.optimizer;

import cn.edu.bit.newnewcc.backend.asm.AsmFunction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmJump;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmLabel;
import cn.edu.bit.newnewcc.backend.asm.operand.Label;

import java.util.*;

public class BranchEliminationOptimizer implements Optimizer {
    @Override
    public boolean runOn(AsmFunction function) {
        List<AsmInstruction> instrList = function.getInstrList();

        Map<Label, Label> labelMap = new HashMap<>();

        for (int i = 0; i < instrList.size() - 1; ++i) {
            AsmInstruction curInstr = instrList.get(i);
            AsmInstruction nextInstr = instrList.get(i + 1);

            if (curInstr instanceof AsmLabel && nextInstr instanceof AsmJump && ((AsmJump) nextInstr).getCondition() == AsmJump.Condition.UNCONDITIONAL) {
                labelMap.put(((AsmLabel) curInstr).getLabel(), (Label) nextInstr.getOperand(1));
            }
        }

        int count = 0;

        for (AsmInstruction instr : instrList) {
            if (instr instanceof AsmJump) {
                for (int i = 1; i <= 3; ++i) {
                    if (instr.getOperand(i) instanceof Label label) {
                        Label newLabel = label;
                        Set<Label> visited = new HashSet<>();

                        while (labelMap.containsKey(newLabel) && !visited.contains(labelMap.get(newLabel))) {
                            newLabel = labelMap.get(newLabel);
                            visited.add(newLabel);
                        }

                        if (!newLabel.equals(label)) {
                            instr.setOperand(i, newLabel);
                            ++count;
                        }
                    }
                }
            }
        }

        return count > 0;
    }
}
