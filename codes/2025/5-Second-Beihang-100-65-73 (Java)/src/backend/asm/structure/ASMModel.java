package backend.asm.structure;

import java.util.List;

public class ASMModel {
    private final List<ASMGlobalObject> globalObjectlist;
    private final List<ASMFunction> functionList;
    private boolean hasParallel = false;

    public ASMModel(List<ASMGlobalObject> globalObjectlist, List<ASMFunction> functionList, boolean hasParallel) {
        this.globalObjectlist = globalObjectlist;
        this.functionList = functionList;
        this.hasParallel = hasParallel;
    }

    public List<ASMGlobalObject> getGlobalObjectlist() {
        return globalObjectlist;
    }

    public List<ASMFunction> getFunctionList() {
        return functionList;
    }

    public void addGlobalObject(ASMGlobalObject globalObject) {
        this.globalObjectlist.add(globalObject);
    }

    public boolean hasParallel() {
        return hasParallel;
    }
}
