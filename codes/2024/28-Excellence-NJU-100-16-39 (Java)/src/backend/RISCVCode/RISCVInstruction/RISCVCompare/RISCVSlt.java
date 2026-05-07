package backend.RISCVCode.RISCVInstruction.RISCVCompare;

import backend.RISCVCode.RISCVInstruction.RISCVInstruction;
import backend.RISCVCode.RISCVOperand;

public class RISCVSlt extends RISCVInstruction {
    private RISCVOperand dest;

    private RISCVOperand operand1;

    private RISCVOperand operand2;

    public RISCVSlt(RISCVOperand dest, RISCVOperand operand1, RISCVOperand operand2) {
        this.dest = dest;
        this.operand1 = operand1;
        this.operand2 = operand2;
    }

    @Override
    public String getString() {
        if (operand1.getName().startsWith("f"))
            return  "  flt.s " + dest.getName() + ", " + operand1.getName() + ", " + operand2.getName() + "\n";
        return  "  slt " + dest.getName() + ", " + operand1.getName() + ", " + operand2.getName() + "\n";
    }
}
