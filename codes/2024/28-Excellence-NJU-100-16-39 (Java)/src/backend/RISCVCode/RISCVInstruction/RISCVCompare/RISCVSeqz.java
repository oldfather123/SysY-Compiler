package backend.RISCVCode.RISCVInstruction.RISCVCompare;

import backend.RISCVBuilder;
import backend.RISCVCode.RISCVInstruction.RISCVInstruction;
import backend.RISCVCode.RISCVOperand;

public class RISCVSeqz extends RISCVInstruction {

    private RISCVOperand dest;

    private RISCVOperand operand;


    public RISCVSeqz(RISCVOperand dest, RISCVOperand operand) {
        this.dest = dest;
        this.operand = operand;
    }

    @Override
    public String getString() {
        return  "  seqz " + dest.getName() + ", " + operand.getName()+ "\n";
    }
}
