package backend.Pass;

import backend.instructions.BackBranch;
import backend.instructions.BackInstruction;
import backend.tools.BackBlock;
import backend.tools.BackFunction;
import backend.tools.BackModule;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.LinkedList;

public class Straighten {
    BackModule backModule;

    public Straighten(BackModule backModule) {
        this.backModule = backModule;
    }

    public void optimize() {
        for (BackFunction f : backModule.getFunctions()) {
            blockSort(f);
            deadJumpElim(f);
        }
    }

    private void blockSort(BackFunction f) {
        ArrayList<BackBlock> sortedBlocks = new ArrayList<>();
        LinkedList<BackBlock> queue = new LinkedList<>();
        HashSet<BackBlock> visited = new HashSet<>();
        queue.add(f.getBackBlocks().get(0));
        visited.add(f.getBackBlocks().get(0));
        f.getBackBlocks().remove(0);
        while (!queue.isEmpty()) {
            BackBlock block = queue.poll();
            sortedBlocks.add(block);

            BackInstruction backInstruction = block.getBackInstructions().getLast();
            if (backInstruction instanceof BackBranch branch && branch.getDstBlock() != null && !visited.contains(branch.getDstBlock())) {
                queue.add(branch.getDstBlock());
                f.getBackBlocks().remove(branch.getDstBlock());
                visited.add(branch.getDstBlock());
            } else {
                for (BackBlock backBlock : f.getBackBlocks()) {
                    if (!visited.contains(backBlock)) {
                        visited.add(backBlock);
                        queue.add(backBlock);
                        f.getBackBlocks().remove(backBlock);
                        break;
                    }
                }
            }
        }
        f.getBackBlocks().clear();
        f.getBackBlocks().addAll(sortedBlocks);
    }


    private void deadJumpElim(BackFunction backFunction) {
        ArrayList<BackBlock> backBlocks = backFunction.getBackBlocks();
        for (int i = 0; i < backBlocks.size() - 1; i++) {
            BackBlock block = backBlocks.get(i);
            BackInstruction lastInstruction = block.getBackInstructions().getLast();
            if (lastInstruction instanceof BackBranch branch &&
                    backBlocks.get(i + 1).equals(branch.getDstBlock())) {
                block.getBackInstructions().remove(block.getBackInstructions().size() - 1);
            }
        }
    }
}
