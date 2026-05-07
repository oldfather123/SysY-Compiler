package backend.instruction;

import backend.component.ObjInstr;
import backend.operand.ObjOperand;

public class ObjNot extends ObjInstr {
    private ObjOperand val;
    private ObjOperand result;

    public ObjNot(ObjOperand val, ObjOperand result) {
        super("not");
        this.val = val;
        this.result = result;
        addDefReg(this.result, result);
        addUseReg(this.val, val);
    }

    public ObjOperand getVal() {
        return this.val;
    }

    public ObjOperand getResult() {
        return this.result;
    }

    @Override
    public String toString() {
        return "not\t" + this.result.toString() + ",\t" + this.val.toString();
    }

    @Override
    public void replaceReg(ObjOperand oldReg, ObjOperand newReg) {
        if (result.equals(oldReg)) {
            result = newReg;
        }
        if (val.equals(oldReg)) {
            val = newReg;
        }
    }
}
