package backend.optimizer;

import backend.RISCVCode.RISCVBlock;
import backend.RISCVCode.RISCVFunction;
import backend.RISCVCode.RISCVInstruction.RISCVInstruction;
import backend.RISCVCode.RISCVInstruction.RISCVJ;
import backend.RISCVCode.RISCVInstruction.RISCVBeqz;

import java.util.*;

public class RemoveUnreachableBlock implements OptForRISCV {
    /**
     * 移除不可达的块
     */
    @Override
    public void Optimize(RISCVFunction riscvFunction) {
        List<RISCVBlock> blocks = riscvFunction.getBlocks();
        if (blocks.isEmpty()) return;

        // 使用广度优先搜索找到所有可达的块
        Set<RISCVBlock> reachableBlocks = new HashSet<>();
        Queue<RISCVBlock> queue = new LinkedList<>();
        RISCVBlock entryBlock = blocks.get(0); // 假设入口块总是可达的
        reachableBlocks.add(entryBlock);
        queue.add(entryBlock);

        while (!queue.isEmpty()) {
            RISCVBlock currentBlock = queue.poll();
            for (RISCVInstruction instruction : currentBlock.getInstructions()) {
                RISCVBlock targetBlock = getTargetBlock(instruction, blocks);
                if (targetBlock != null && !reachableBlocks.contains(targetBlock)) {
                    reachableBlocks.add(targetBlock);
                    queue.add(targetBlock);
                }
            }
        }

        // 移除不可达的块
        blocks.removeIf(block -> !reachableBlocks.contains(block));
    }

    private RISCVBlock getTargetBlock(RISCVInstruction instruction, List<RISCVBlock> blocks) {
        if (instruction instanceof RISCVJ) {
            String targetLabel = ((RISCVJ) instruction).getLabel().getName();
            return findBlockByLabel(targetLabel, blocks);
        } else if (instruction instanceof RISCVBeqz) {
            String targetLabel = ((RISCVBeqz) instruction).getLabel().getName();
            return findBlockByLabel(targetLabel, blocks);
        }
        return null;
    }

    private RISCVBlock findBlockByLabel(String label, List<RISCVBlock> blocks) {
        for (RISCVBlock block : blocks) {
            if (block.getLabel().getName().equals(label)) {
                return block;
            }
        }
        return null;
    }
}