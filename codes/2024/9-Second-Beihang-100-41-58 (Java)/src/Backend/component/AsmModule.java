package Backend.component;

import midend.Function;

import java.util.ArrayList;

public class AsmModule {
    private final ArrayList<AsmFunction> functions;
    private final  ArrayList<AsmGlobalVariable> globalVariables;

    public AsmModule() {
        this.globalVariables = new ArrayList<>();
        this.functions = new ArrayList<>();
    }

    public ArrayList<AsmFunction> getFunctions() {
        return functions;
    }

    public ArrayList<AsmGlobalVariable> getGlobalVariables() {
        return globalVariables;
    }

    public void addGlobalVariable(AsmGlobalVariable objGlobalVariable) {
        globalVariables.add(objGlobalVariable);
    }

    public void addFunction(AsmFunction objFunction) {
        functions.add(objFunction);
    }

    public AsmFunction getObjFunction(Function irFunction) {
        for (AsmFunction objF : functions) {
            if (objF.isThis(irFunction.getName())) {
                return objF;
            }
        }
        return null;
    }
}
