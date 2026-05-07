package mid.IntermediatePresentation.Instruction;

import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;

public class Fptosi extends Instruction {
    public Fptosi(Value v) {
        super(IRManager.getInstance().declareTempVar(), ValueType.I32);
        use(v);
    }

    public String toString() {
        return reg + " = fptosi float " + operandList.get(0).getReg() + " to i32\n";
    }
}
