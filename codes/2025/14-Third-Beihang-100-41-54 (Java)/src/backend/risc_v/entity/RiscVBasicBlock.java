package backend.risc_v.entity;

import backend.risc_v.entity.instrcution.RiscVInstruction;

import java.util.ArrayList;

public class RiscVBasicBlock {

    private final String label;
    private final ArrayList<RiscVInstruction> instructions;

    public RiscVBasicBlock(String label) {
        this.label = label;
        this.instructions = new ArrayList<>();
    }

    public String getLabel() {
        return label;
    }

    public void addInstruction(RiscVInstruction instruction) {
        instructions.add(instruction);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(label).append(":\n");
        for (RiscVInstruction instruction : instructions) {
            sb.append("\t").append(instruction).append("\n");
        }
        return sb.toString();
    }
}
