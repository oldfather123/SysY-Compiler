package mid.IntermediatePresentation.Instruction;

import mid.IntermediatePresentation.Value;

public class Push extends Instruction {
    private final int offset;

    public Push(Value v, int offset) {
        super("PUSH");
        use(v);
        this.offset = offset;
    }

    public String toString() {
        return "push " + getOperandList().get(0).getReg() + ", " + offset + "\n";
    }

    public Value getValue() {
        return operandList.get(0);
    }

    public int getOffset() {
        return offset;
    }
}
