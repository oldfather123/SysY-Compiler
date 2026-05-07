package frontend.ir.structure;

import frontend.ir.symbol.GlbObjPtr;
import frontend.ir.symbol.SymTab;
import frontend.ir.symbol.Symbol;

import java.util.List;

public class IRModel {
    private final List<Function> functions;
    private final SymTab globalSymTab;
    private boolean hasParallel = false;

    public List<Function> getFunctions() {
        return functions;
    }

    public void addFunction(Function function) {
        functions.add(function);
    }

    public void addFunctionToHead(Function function) {
        functions.addFirst(function);
    }

    public void addGlobalValue(Symbol globalValue) {
        globalSymTab.addObject(globalValue, -1);
    }

    public SymTab getGlobalSymTab() {
        return globalSymTab;
    }

    public void setHasParallel(boolean hasParallel) {
        this.hasParallel = hasParallel;
    }

    public boolean hasParallel() {
        return hasParallel;
    }

    public IRModel(List<Function> functions, SymTab globalSymTab) {
        this.functions = functions;
        this.globalSymTab = globalSymTab;
    }
}
