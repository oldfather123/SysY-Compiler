package backend.instruction;

import backend.component.ObjInstr;
import backend.operand.ObjOperand;

public class ObjTrunc extends ObjInstr {
    private ObjOperand result;
    private ObjOperand value;

    public ObjTrunc(ObjOperand result, ObjOperand value) {
        super("trunc");
        this.value = value;
        this.result = result;
    }

    @Override
    public String toString() {
        return "Error: You are using trunc instruction!";
    }
}
