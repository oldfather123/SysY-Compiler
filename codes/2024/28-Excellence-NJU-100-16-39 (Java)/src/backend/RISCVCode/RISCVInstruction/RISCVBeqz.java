package backend.RISCVCode.RISCVInstruction;

import backend.RISCVCode.RISCVOperand;

public class RISCVBeqz extends RISCVInstruction{

    private final RISCVOperand operand;

    private final RISCVOperand label;

    public RISCVBeqz(RISCVOperand operand, RISCVOperand label) {
        this.operand = operand;
        this.label = label;
    }

    @Override
    public String getString() {
        return "  beqz " + operand.getName() + ", " + label.getName() + "\n";
    }

    public RISCVOperand getLabel() {
        return this.label;
    }
}
