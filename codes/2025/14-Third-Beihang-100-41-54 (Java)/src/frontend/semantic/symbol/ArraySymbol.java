package frontend.semantic.symbol;

import backend.ir.entity.ArrayValue;

import java.util.List;

public class ArraySymbol extends Symbol {
    private final VarInfo varInfo;
    private ArrayValue arrayInitValue = null;

    public VarInfo getVarInfo() {
        return varInfo;
    }

    public ArrayValue getArrayInitValue() {
        return arrayInitValue;
    }

    public void setArrayInitValue(ArrayValue arrayInitValue) {
        this.arrayInitValue = arrayInitValue;
    }

    public boolean isArrayConst(){return arrayInitValue != null;}

    public ArraySymbol(String name, IRBasicType type, boolean isConst, boolean isGlobal, List<Integer> dim) {
        super(name, type, isGlobal, isConst);
        varInfo = new VarInfo(type, isConst, dim != null, dim);
    }

    public List<Integer> getDim() {
        return varInfo.getFullDim();
    }
}
