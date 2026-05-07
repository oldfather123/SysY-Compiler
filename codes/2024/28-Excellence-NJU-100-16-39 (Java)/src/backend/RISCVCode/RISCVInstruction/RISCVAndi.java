package backend.RISCVCode.RISCVInstruction;
import backend.RISCVCode.RISCVOperand;

public class RISCVAndi extends RISCVInstruction{
    private final RISCVOperand dest;

    private final RISCVOperand operand;

    private final int positions;


    public RISCVAndi(RISCVOperand dest, RISCVOperand operand, int positions) {
        this.dest = dest;
        this.operand = operand;
        this.positions = positions;
    }

    @Override
    public String getString() {
        return "  andi " + dest.getName() + ", " + operand.getName() + ", " + positions + "\n";
    }
}
