package backend.RISCVCode;

import backend.RISCVCode.RISCVInstruction.RISCVInstruction;

import java.util.ArrayList;
import java.util.List;

/**
 * RISCVBlock类，包含多个RISCV指令（RISCVInstruction）
 */
public class RISCVBlock {
    RISCVLabel blockLabel;

    ArrayList<RISCVInstruction> instructions;

    public RISCVBlock(String name) {
        blockLabel = new RISCVLabel(name);
        instructions = new ArrayList<>();
    }

    public RISCVOperand getLabel() {return this.blockLabel;}

    public void addInstruction(RISCVInstruction instruction) {
        instructions.add(instruction);
    }

    public void insertInstruction(int index, RISCVInstruction instruction) {
        instructions.add(index, instruction);
    }

    public List<RISCVInstruction> getInstructions() {
        return instructions;
    }

    public RISCVLabel getBlockLabel() {
        return blockLabel;
    }
}
