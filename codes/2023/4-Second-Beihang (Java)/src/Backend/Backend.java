package Backend;

import Backend.component.ObjModule;
import Backend.process.IrParser;
import IR.IRModule;

public class Backend {

    private ObjModule objModule;
    public Backend(IRModule irModule) {
        IrParser irParser = new IrParser(irModule);
        this.objModule = irParser.parseModule();

    }
    public ObjModule getModule() {
        return objModule;
    }
}
