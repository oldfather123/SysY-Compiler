package mid.IntermediatePresentation.Instruction;

import mid.IntermediatePresentation.ValueType;

public class GetParam extends Instruction {
    private final int offset;

    public GetParam(String param, int offset, ValueType valueType) {
        super(param, valueType);
        this.offset = offset;
    }

    public int getOffset() {
        return offset;
    }

    public String toString() {
        return reg + " = getParam " + offset + "\n";
    }
}
