package backend.instruction;

import backend.component.ObjFunction;
import backend.component.ObjInstr;
import backend.operand.ObjImm;
import backend.operand.ObjOperand;
import backend.operand.ObjPhyReg;

public class ObjRet extends ObjInstr {
    private ObjOperand value;
    private boolean isVoid;
    private ObjFunction objFunction;

    public ObjRet(ObjOperand value) {
        super("ret");
        this.value = value;
        this.isVoid = false;
    }

    public ObjRet() {
        super("ret");
        this.isVoid = true;
    }

    public void setObjFunction(ObjFunction objFunction) {
        this.objFunction = objFunction;
    }

    @Override
    public String toString() {
        return "ret";
    }
}
