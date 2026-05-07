package Backend.instruction;

import Backend.component.ObjFunction;

public class ObjCall extends ObjInstr {
    public ObjFunction tarFunction;
    public ObjCall(ObjFunction objFunction) {
        this.tarFunction = objFunction;
    }
    @Override
    public String toString() {
        return "call\t" + tarFunction.getName();
    }
}
