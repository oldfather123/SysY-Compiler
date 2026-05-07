package backend.RISCVCode;

public class RISCVLabel extends RISCVOperand{
    public RISCVLabel(String name) {
        super(OperandType.label, name);
    }
}
