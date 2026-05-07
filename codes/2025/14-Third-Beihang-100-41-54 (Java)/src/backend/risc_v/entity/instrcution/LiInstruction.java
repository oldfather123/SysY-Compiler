package backend.risc_v.entity.instrcution;

import backend.risc_v.entity.register.Register;

public class LiInstruction extends RiscVInstruction{

    private final Register rd;
    private int immediateValue = 0;

    public LiInstruction(Register rd, int immediateValue) {
        this.rd = rd;
        this.immediateValue = immediateValue;
    }

    @Override
    public String toString() {
        return   "li " + rd + ", " + immediateValue;
    }
}
