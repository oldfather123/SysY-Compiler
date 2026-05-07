package backend.tools;

import backend.instructions.BackAnnotation;
import backend.instructions.BackInstruction;
import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.Instruction.Instruction;

import java.util.ArrayList;
import java.util.LinkedList;

public class BackBlock {
    private final BasicBlock midBlock;
    private String name;
    private LinkedList<BackInstruction> backInstructions;

    public BackBlock(BasicBlock midBlock) {
        this.name = midBlock.getFunction().getName() + "_" + midBlock.getReg();
        backInstructions = new LinkedList<>();
        this.midBlock = midBlock;
    }

    public void addInstruction(BackInstruction backInstruction) {
        backInstructions.add(backInstruction);
    }

    public void addFirstInstruction(BackInstruction backInstruction) {
        backInstructions.addFirst(backInstruction);
    }

    public void addBefore(BackInstruction backInstruction) {
        backInstructions.add(backInstructions.size() - 1, backInstruction);
    }

    public LinkedList<BackInstruction> getBackInstructions() {
        return backInstructions;
    }

    public BasicBlock getMidBlock() {
        return midBlock;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(name).append(":\n");
        for (BackInstruction backInstruction : backInstructions) {
            if (!(backInstruction instanceof BackAnnotation)) {
                sb.append(" ").append(backInstruction).append("\n");
            } else {
                sb.append(" ").append(backInstruction);
            }
        }
        return sb.toString();
    }

    public String getName() {
        return name;
    }
}
