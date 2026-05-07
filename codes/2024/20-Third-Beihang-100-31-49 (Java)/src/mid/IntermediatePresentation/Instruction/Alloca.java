package mid.IntermediatePresentation.Instruction;

import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.ValueType;

import java.util.ArrayList;

public class Alloca extends Instruction {

    public Alloca(boolean isInt) {
        super(IRManager.getInstance().declareVar(), isInt ? ValueType.PI32 : ValueType.PFLT);
        if (IRManager.getInstance().autoInsert()) {
            getBlock().removeInstruction(this);
            setBlock(getBlock().getFunction().getEntranceBlock());
            getBlock().addInstrAtEntry(this);
        }
    }

    public Alloca(ValueType vType) {
        // 这个不会在中端使用，只会在reg alloca使用
        super(IRManager.getInstance().declareVar(), vType.getPointerType());
    }

    public Alloca(int len, boolean isInt) {
        super(IRManager.getInstance().declareVar(), ValueType.ARRAY);
        vType = new ValueType(len, isInt);
        if (IRManager.getInstance().autoInsert()) {
            getBlock().removeInstruction(this);
            setBlock(getBlock().getFunction().getEntranceBlock());
            getBlock().addInstrAtEntry(this);
        }
    }

    public String toString() {
        if (vType.equals(ValueType.ARRAY)) {
            return reg + " = alloca " + vType + "\n";
        } else {
            return reg + " = alloca " + vType.getRefTypeString() + "\n";
        }
    }


    public ArrayList<String> GVNHash() {
        return null;
    }

    public int getLen() {
        return vType.getLength();
    }

    public boolean isDefInstr() {
        return false;
    }
}
