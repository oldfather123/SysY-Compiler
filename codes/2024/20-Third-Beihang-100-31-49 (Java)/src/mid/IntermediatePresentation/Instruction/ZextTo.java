package mid.IntermediatePresentation.Instruction;

import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;
public class ZextTo extends Instruction {
    public ZextTo(Value v, ValueType type) {
        super(IRManager.getInstance().declareTempVar(), type);
        use(v);
    }

    public String toString() {
        return reg + " = zext " + operandList.get(0).getTypeString() + " " +
                operandList.get(0).getReg() + " to " + getTypeString() + "\n";
    }
}
