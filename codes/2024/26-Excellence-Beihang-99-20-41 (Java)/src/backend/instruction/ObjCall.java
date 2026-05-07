package backend.instruction;

import backend.component.ObjFunction;
import backend.component.ObjInstr;
import backend.operand.ObjOperand;

import java.util.List;

public class ObjCall extends ObjInstr {
    ObjOperand value;
    ObjOperand elseValue;
    private String functionName;

    public ObjCall(ObjOperand value, ObjOperand elseValue) {
        super("Call");
        this.value = value;
        this.elseValue = elseValue;
    }

    public ObjCall(String functionName) {
        super("call");
        this.functionName = functionName;
    }

    @Override
    public String toString() {
        // Just remove the @ symbol
        return "call\t" + functionName.replace("@", "");
    }
}
