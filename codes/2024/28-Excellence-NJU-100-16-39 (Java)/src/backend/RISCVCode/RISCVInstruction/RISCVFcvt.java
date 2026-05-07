package backend.RISCVCode.RISCVInstruction;

import backend.RISCVCode.RISCVOperand;

public class RISCVFcvt extends RISCVInstruction{
    private final RISCVOperand dest;

    private final RISCVOperand operand1;

    private final String mode;

    public RISCVFcvt(RISCVOperand dest, RISCVOperand operand1, String mode) {
        this.dest = dest;
        this.operand1 = operand1;
        this.mode = mode;
    }
    @Override
    public String getString() {
        if (dest.getName().startsWith("f"))
            return "  fcvt.s.w " + dest.getName() + ", " + operand1.getName() + "\n";
        return "  fcvt.w.s " + dest.getName() + ", " + operand1.getName() + ", " + mode + "\n";
    }
}
