package backend.instruction;

import backend.component.ObjInstr;
import backend.operand.ObjOperand;

public class ObjConversion extends ObjInstr {
    private ObjOperand dst, src;

    public ObjConversion(String type, ObjOperand dst, ObjOperand src) {
        super(type);
        this.dst = dst;
        this.src = src;
        addDefReg(this.dst, dst);
        addUseReg(this.src, src);
    }

    public ObjOperand getDst() {
        return dst;
    }

    @Override
    public String toString() {
        String frm = getType().equals("fcvt.w.s") ? ",\trtz" : "";
        return getType() + "\t" + dst + ",\t" + src + frm;
    }

    @Override
    public void replaceReg(ObjOperand oldReg, ObjOperand newReg) {
        if (dst.equals(oldReg)) {
            dst = newReg;
        }
        if (src.equals(oldReg)) {
            src = newReg;
        }
    }
}
