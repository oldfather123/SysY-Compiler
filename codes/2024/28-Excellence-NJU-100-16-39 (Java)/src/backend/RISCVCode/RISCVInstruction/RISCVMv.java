package backend.RISCVCode.RISCVInstruction;

import backend.RISCVCode.RISCVOperand;

public class RISCVMv extends RISCVInstruction {
    private final RISCVOperand dest;

    private final RISCVOperand operand1;

    private final String opt;

    public RISCVMv(RISCVOperand dest, RISCVOperand operand1) {
        this.dest = dest;
        this.operand1 = operand1;
        if (dest.getName().startsWith("f") && operand1.getName().startsWith("f"))
            opt = "fmv.s";
        else if (dest.getName().startsWith("f") &&!operand1.getName().startsWith("f"))
            opt = "fmv.w.x";
        else if (!dest.getName().startsWith("f") &&operand1.getName().startsWith("f"))
            opt = "fmv.x.w";
        else
            opt = "mv";
    }

    @Override
    public String getString() {
        return "  " + opt + " " + dest.getName() + ", " + operand1.getName() + "\n";
    }

    public RISCVOperand getSrc() {
        return this.operand1;
    }

    public RISCVOperand getDest() {
        return this.dest;
    }
}
