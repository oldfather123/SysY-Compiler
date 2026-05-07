package mid.SymbolTable;

import mid.IntermediatePresentation.Instruction.GlobalDecl;
import mid.IntermediatePresentation.Value;

import java.util.ArrayList;
import java.util.HashMap;

public class VarElem extends SymbolTableElem {
    private final boolean isConst;
    private final int dim;
    private final ArrayList<Integer> lens;

    private Number val = null;

    private Value irValue = null;

    private Number globalVal = null;

    private HashMap<Integer, Number> elemList = new HashMap<>();

    public VarElem(boolean isConst, int dim, ArrayList<Integer> lens) {
        super();
        this.isConst = isConst;
        this.dim = dim;
        this.lens = lens;
    }

    public void arrayInit(HashMap<Integer, Number> elemList) {
        this.elemList.putAll(elemList);
    }

    public int getDim() {
        return dim;
    }

    public ArrayList<Integer> getLens() {
        return lens;
    }

    public boolean isConst() {
        return isConst;
    }

    public void setVal(Number val) {
        this.val = val;
        this.globalVal = val;
    }

    public Number getVal() {
        return val;
    }

    public void setArrayVal(Number val, int index) {
        elemList.put(index, val);
    }

    public Number getArrayVal(int index) {
        return elemList.getOrDefault(index, 0);
    }

    public void setIrValue(Value irValue) {
        this.irValue = irValue;
    }

    public Value getIrValue() {
        return irValue;
    }

    public void setTempVal(Number val) {
        if (irValue instanceof GlobalDecl) {
            globalVal = this.val;
        }
        this.val = val;
    }

    public void resetValForGlobalVar() {
        if (irValue instanceof GlobalDecl) {
            this.val = globalVal;
        }
    }
}
