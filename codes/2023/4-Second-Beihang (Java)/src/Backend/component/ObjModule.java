package Backend.component;

import java.util.ArrayList;

public class ObjModule {
    private ArrayList<ObjGlobalVariable> globalVariables;
    private ArrayList<ObjFunction> functions;
    private ObjFunction mainFunction;

    public ObjModule() {
        globalVariables = new ArrayList<>();
        functions = new ArrayList<>();
    }
    public ArrayList<ObjFunction> getFunctions() {
        return functions;
    }
    public ArrayList<ObjGlobalVariable> getGlobalVariables() { return globalVariables; }

    public void addGlobalVariable(ObjGlobalVariable g) {
        globalVariables.add(g);
    }
    public void addFunction(ObjFunction f) {
        functions.add(f);
        if(f.getName() == "main")
            mainFunction = f;
    }
    public void print() {
        for(ObjFunction f : functions)
            f.print();
    }
}
