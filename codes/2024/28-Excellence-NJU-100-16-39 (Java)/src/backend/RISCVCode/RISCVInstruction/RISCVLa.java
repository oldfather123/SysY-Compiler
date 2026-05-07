package backend.RISCVCode.RISCVInstruction;

import backend.RISCVCode.RISCVOperand;

public class RISCVLa extends RISCVInstruction {
    private final RISCVOperand dest;

    private final RISCVOperand operand1;

    public RISCVLa(RISCVOperand dest, RISCVOperand operand1) {
        this.dest = dest;
        this.operand1 = operand1;
    }
    @Override
    public String getString() {
        return "  la " + dest.getName() + ", " + operand1.getName() + "\n";
    }
}
