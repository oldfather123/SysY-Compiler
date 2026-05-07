package mid.SymbolTable;

import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.ValueType;

public class FuncElem extends SymbolTableElem {
    private final ValueType retType;

    private Function functionIR;


    public FuncElem(ValueType retType) {
        super();
        this.retType = retType;
    }

    public void setFunctionIR(Function function) {
        this.functionIR = function;
    }

    public Function getFunctionIR() {
        return functionIR;
    }

    public ValueType getRetType() {
        return retType;
    }


}
