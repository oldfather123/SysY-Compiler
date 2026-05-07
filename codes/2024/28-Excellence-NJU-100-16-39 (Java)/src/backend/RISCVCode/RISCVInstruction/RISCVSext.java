package backend.RISCVCode.RISCVInstruction;

import backend.RISCVCode.RISCVOperand;

public class RISCVSext extends RISCVInstruction {
    private final RISCVOperand dest;

    private final RISCVOperand operand1;

    private final String opt;

    public RISCVSext(RISCVOperand dest, RISCVOperand operand1) {
        this.dest = dest;
        this.operand1 = operand1;
        opt = "sext.w";
    }

    @Override
    public String getString() {
        return "  " + opt + " " + dest.getName() + ", " + operand1.getName() + "\n";
    }
}
