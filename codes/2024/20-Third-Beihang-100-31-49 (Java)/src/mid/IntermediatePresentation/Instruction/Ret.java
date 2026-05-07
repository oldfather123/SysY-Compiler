package mid.IntermediatePresentation.Instruction;

import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Function.MainFunction;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;

public class Ret extends Instruction {

    public Ret() {
        super("RET", ValueType.NULL);
    }

    public Ret(Value retValue, boolean isInt) {
        super("RET", isInt ? ValueType.I32 : ValueType.FLT);
        use(retValue);
    }

    public String toString() {
        if (vType == ValueType.NULL) {
            return "ret void\n";
        } else {
            return "ret " + getTypeString() + " " + operandList.get(0).getReg() + "\n";
        }
    }

    public boolean isUseless() {
        return false;
    }

    public boolean isDefInstr() {
        return false;
    }

    public Value getRetValue() {
        return operandList.get(0);
    }
}
