package backend.RISCVCode.RISCVInstruction;

import backend.RISCVCode.RISCVOperand;

public class RISCVJ extends RISCVInstruction{
    private RISCVOperand label;

    public RISCVJ(RISCVOperand label) {
        this.label = label;
    }

    public RISCVOperand getLabel() {return label;}

    @Override
    public String getString() {
        return "  j " + label.getName() + "\n";
    }
}
