package backend.itemStructure;

import java.util.ArrayList;

public class AsmModule {
    private ArrayList<AsmGlobalVar> globalVar;
    private ArrayList<AsmFunction> functions;
    private AsmFunction mainFunction;

    public AsmModule() {
        this.globalVar = new ArrayList<>();
        this.functions = new ArrayList<>();
    }

    public void addGlobalVar(AsmGlobalVar globalVar) {
        this.globalVar.add(globalVar);
    }
    public void addFunction(AsmFunction function) {
        this.functions.add(function);
    }
    public ArrayList<AsmGlobalVar> getGlobalVars() {
        return globalVar;
    }
    public ArrayList<AsmFunction> getFunctions() {
        return functions;
    }
}
