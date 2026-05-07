package cn.edu.bit.newnewcc.backend.asm.optimizer;

import cn.edu.bit.newnewcc.backend.asm.AsmFunction;
import cn.edu.bit.newnewcc.backend.asm.instruction.*;
import cn.edu.bit.newnewcc.backend.asm.operand.Label;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class BlockInlineOptimizer implements Optimizer {
    private final int threshold;

    public BlockInlineOptimizer(int threshold) {
        this.threshold = threshold;
    }

    @Override
    public boolean runOn(AsmFunction function) {
        List<AsmInstruction> instrList = function.getInstrList();

        int count = 0;

        boolean madeChange;
        do {
            madeChange = false;

            Map<Label, List<AsmInstruction>> blocks = computeBlocks(instrList);
            Map<Label, Label> fallThroughLabels = computeFallThroughLabels(instrList);

            List<AsmInstruction> newInstrList = new ArrayList<>();
            for (AsmInstruction instr : instrList) {
                boolean inline = false;
                if (instr instanceof AsmJump jumpInstr
                    && jumpInstr.getCondition() == AsmJump.Condition.UNCONDITIONAL) {
                    Label label = (Label) jumpInstr.getOperand(1);
                    List<AsmInstruction> block = blocks.get(label);
                    if (block.size() <= threshold) {
                        newInstrList.addAll(block);
                        if (fallThroughLabels.containsKey(label)) {
                            newInstrList.add(AsmJump.createUnconditional(fallThroughLabels.get(label)));
                        }
                        inline = true;
                        madeChange = true;
                        ++count;
                    }
                }
                if (!inline) {
                    newInstrList.add(instr);
                }
            }

            instrList.clear();
            instrList.addAll(newInstrList);
        } while (madeChange);

        return count > 0;
    }

    private static Map<Label, List<AsmInstruction>> computeBlocks(List<AsmInstruction> instrList) {
        Map<Label, List<AsmInstruction>> blocks = new HashMap<>();
        Label label = null;
        for (AsmInstruction instr : instrList) {
            if (instr instanceof AsmLabel labelInstr) {
                label = labelInstr.getLabel();
                blocks.put(label, new ArrayList<>());
            } else if (!(instr instanceof AsmBlockEnd)) {
                if (label != null) blocks.get(label).add(instr);
            }
        }
        return blocks;
    }

    private static Map<Label, Label> computeFallThroughLabels(List<AsmInstruction> instrList) {
        Map<Label, Label> fallThroughLabels = new HashMap<>();

        Label label = null;
        boolean fallThrough = true;
        for (AsmInstruction instr : instrList) {
            if (instr instanceof AsmLabel labelInstr) {
                Label newLabel = labelInstr.getLabel();
                if (label != null && fallThrough) {
                    fallThroughLabels.put(label, newLabel);
                }
                label = newLabel;
                fallThrough = true;
            } else if (!(instr instanceof AsmBlockEnd) && instr.willNeverReturn()) {
                fallThrough = false;
            }
        }

        return fallThroughLabels;
    }
}
