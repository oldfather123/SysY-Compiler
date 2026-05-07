package mid.IntermediatePresentation.Instruction;

import mid.IntermediatePresentation.Array.ArrayInitializer;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;

import java.util.ArrayList;

public class GlobalDecl extends Instruction {
    private boolean isConst = false;

    public GlobalDecl(Value val, boolean isInt) {
        super(IRManager.getInstance().declareVar(), isInt ? ValueType.PI32 : ValueType.PFLT);
        use(val);
        IRManager.getModule().addGobalDecl(this);
        if (val instanceof ArrayInitializer aInit) {
            aInit.setType(vType);
            vType = aInit.getType();
        }
    }

    public GlobalDecl(Value val, boolean isInt, boolean isConst) {
        super(IRManager.getInstance().declareGlobalVar(), isInt ? ValueType.PI32 : ValueType.PFLT);
        use(val);
        IRManager.getModule().addGobalDecl(this);
        if (val instanceof ArrayInitializer aInit) {
            aInit.setType(vType);
            vType = aInit.getType();
        }
        this.isConst = isConst;
    }

    public String toString() {
        Value init = operandList.get(0);
        if (init instanceof ArrayInitializer aInit) {
            return reg + " = dso_local global " + aInit.getType() + " " + aInit + "\n";
        } else {
            return reg + " = dso_local global " + vType.getRefType() + " " + init + "\n";
        }
    }


    public boolean isArray() {
        return operandList.get(0) instanceof ArrayInitializer;
    }

    public Value getInit() {
        return operandList.get(0);
    }

    public boolean isConst() {
        return isConst;
    }

    public Number getConstValAtIndex(int index) {
        Value init = operandList.get(0);
        if (init instanceof ArrayInitializer arrayInitializer) {
            return ((ConstNumber) arrayInitializer.getVals().getOrDefault(index, new ConstNumber(0))).getVal();
        } else {
            throw new RuntimeException();
        }
    }

    public ArrayList<String> GVNHash() {
        return null;
    }

    public void rename(String name) {
        reg = name;
    }
}
