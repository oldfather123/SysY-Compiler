package mid.IntermediatePresentation.Instruction;

import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;

public class Sitofp extends Instruction {
    public Sitofp(Value v) {
        super(IRManager.getInstance().declareTempVar(), ValueType.FLT);
        use(v);
    }

    public String toString() {
        return reg + " = sitofp i32 " + operandList.get(0).getReg() + " to float\n";
    }
}
