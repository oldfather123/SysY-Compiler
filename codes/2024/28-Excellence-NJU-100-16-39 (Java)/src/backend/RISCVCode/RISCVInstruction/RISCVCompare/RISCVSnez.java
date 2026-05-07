package backend.RISCVCode.RISCVInstruction.RISCVCompare;

import backend.RISCVCode.RISCVInstruction.RISCVInstruction;
import backend.RISCVCode.RISCVOperand;

public class RISCVSnez extends RISCVInstruction {
    private RISCVOperand dest;

    private RISCVOperand operand;


    public RISCVSnez(RISCVOperand dest, RISCVOperand operand) {
        this.dest = dest;
        this.operand = operand;

    }

    @Override
    public String getString() {
        return  "  snez " + dest.getName() + ", " + operand.getName()+ "\n";
    }
}
