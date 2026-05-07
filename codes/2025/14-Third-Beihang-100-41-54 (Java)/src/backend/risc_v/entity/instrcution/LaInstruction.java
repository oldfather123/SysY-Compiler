package backend.risc_v.entity.instrcution;

import backend.risc_v.entity.register.Register;

public class LaInstruction extends RiscVInstruction {
    private final Register rd;
    private final String varName;

    public LaInstruction(Register rd, String varName) {
        this.rd = rd;
        this.varName = varName;
    }

    @Override
    public String toString() {
        return "la " + rd + ", " + varName;
    }
}
