package backend.component;

import java.util.ArrayList;

public class ObjModule {
    private ArrayList<ObjGlobalVar> globalVars;
    private ArrayList<ObjFunction> functions;

    public ObjModule() {
        globalVars = new ArrayList<>();
        functions = new ArrayList<>();
    }

    public ArrayList<ObjFunction> getFunctions() {
        return functions;
    }

    public ArrayList<ObjGlobalVar> getGlobalVars() {
        return globalVars;
    }

    public void addGlobalVar(ObjGlobalVar g) {
        globalVars.add(g);
    }

    public void setGlobalVars(ArrayList<ObjGlobalVar> globalVars) {
        this.globalVars = globalVars;
    }

    public void addFunction(ObjFunction f) {
        functions.add(f);
    }

    @Override
    public String toString() {
        StringBuilder string = new StringBuilder();
        for (ObjGlobalVar globalVar : globalVars) {
            string.append(globalVar);
        }
        for (ObjFunction function : functions) {
            if (function.getName().equals("main")) {
                string.append(".section\t.text.startup\n.align\t\t1\n.globl\t\tmain\n");
            } else {
                string.append(".section\t.text\n.align\t\t1\n");
            }
            string.append(function);
        }
        return string.toString();
    }
}
