package backend.RISCVCode.RISCVInstruction;

import backend.RISCVCode.RISCVOperand;

public class RISCVLi extends RISCVInstruction {
    private final RISCVOperand dest;

    private final RISCVOperand operand1;

    public RISCVLi(RISCVOperand dest, RISCVOperand operand1) {
        this.dest = dest;
        this.operand1 = operand1;
    }
    @Override
    public String getString() {
        return "  li " + dest.getName() + ", " + operand1.getName() + "\n";
    }
    public RISCVOperand getOperand1(){return operand1;}
}
