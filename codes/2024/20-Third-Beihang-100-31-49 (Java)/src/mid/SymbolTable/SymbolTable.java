package mid.SymbolTable;

import mid.IntermediatePresentation.Instruction.GlobalDecl;
import mid.IntermediatePresentation.ValueType;

import java.util.ArrayList;
import java.util.HashMap;

public class SymbolTable {
    HashMap<String, SymbolTableElem> elems = new HashMap<>();

    public boolean hasDeclared(String ident) {
        return elems.containsKey(ident);
    }

    public void varDecl(String ident, boolean isConst, int dim, ArrayList<Integer> lens) {
        elems.put(ident, new VarElem(isConst, dim, lens));
    }

    public void funcDecl(ValueType retType, String ident) {
        elems.put(ident, new FuncElem(retType));
    }

    public FuncElem getFunction(String ident) {
        SymbolTableElem elem = elems.getOrDefault(ident, null);
        if (elem == null) {
            return null;
        } else if (!(elem instanceof FuncElem)) {
            return null;
        } else {
            return (FuncElem) elem;
        }
    }

    public VarElem getVar(String ident) {
        SymbolTableElem elem = elems.getOrDefault(ident, null);
        if (elem == null) {
            return null;
        } else if (!(elem instanceof VarElem)) {
            return null;
        } else {
            return (VarElem) elem;
        }
    }

    public void resetValForGlobalVar(){
        for (SymbolTableElem elem:elems.values()){
            if (elem instanceof VarElem v && v.getIrValue() instanceof GlobalDecl){
                v.resetValForGlobalVar();
            }
        }
    }
}
