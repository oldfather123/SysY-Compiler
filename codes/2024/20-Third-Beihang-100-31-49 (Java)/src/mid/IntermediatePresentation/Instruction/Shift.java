package mid.IntermediatePresentation.Instruction;

import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;

public class Shift extends Instruction {
    private final boolean shiftRight;
    private boolean logicalShiftRight = false;

    public Shift(boolean shiftRight, Value v, ConstNumber n) {
        super(IRManager.getInstance().declareTempVar(), ValueType.I32);
        this.shiftRight = shiftRight;
        use(v);
        use(n);
    }

    public String toString() {
        String instr;
        if (shiftRight) {
            if (logicalShiftRight) {
                instr = "lshr";
            } else {
                instr = "ashr";
            }
        } else {
            instr = "shl";
        }
        return reg + " = " + instr + " i32 " + operandList.get(0).getReg() + ", " + operandList.get(1).getReg() + "\n";
    }

    public void setLogicalShiftRight(boolean logicalRight) {
        this.logicalShiftRight = logicalRight;
    }

    public boolean getLogicalShiftRight() {
        return this.logicalShiftRight;
    }

    public boolean isShiftRight() {
        return shiftRight;
    }

    public boolean isLogicalShiftRight() {
        return logicalShiftRight;
    }
}
