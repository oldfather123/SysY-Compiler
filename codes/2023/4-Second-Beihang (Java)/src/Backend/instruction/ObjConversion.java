package Backend.instruction;

import Backend.operand.ObjImm;
import Backend.operand.ObjImm12;
import Backend.operand.ObjOperand;

public class ObjConversion extends ObjInstr {
    private String type;
    private ObjOperand dst, src;

    public ObjConversion(String type, ObjOperand dst, ObjOperand src) {
        this.type = type;
        setDst(dst);
        setSrc(src);
    }
    private void setDst(ObjOperand dst) {
        addDefReg(this.dst, dst);
        this.dst = dst;
    }
    private void setSrc(ObjOperand src) {
        addUseReg(this.src, src);
        this.src = src;
    }
    public String getType() { return type; }
    public ObjOperand getDst() { return dst; }
    public ObjOperand getSrc() { return src; }
    public boolean isSrcImm() { return src instanceof ObjImm; }
    public boolean isSrcImm12() { return src instanceof ObjImm12; }

    @Override
    public void replaceReg(ObjOperand oldReg, ObjOperand newReg) {
        if (dst.equals(oldReg))
            setDst(newReg);
        if (src.equals(oldReg))
            setSrc(newReg);
    }

    @Override
    public void replaceUseReg(ObjOperand oldReg, ObjOperand newReg) {
        if (src.equals(oldReg))
            setSrc(newReg);
    }


    public static ObjConversion getFtoi(ObjOperand dst, ObjOperand src) {
        // fcvt.w.s a5, fa5, rtz
        return new ObjConversion("fcvt.w.s", dst, src);
    }
    public static ObjConversion getItof(ObjOperand dst, ObjOperand src) {
        return new ObjConversion("fcvt.s.w", dst, src);
    }

    @Override
    public String toString() {
        String output = type + "\t" + dst + ",\t" + src;
        if(type == "fcvt.w.s")
            output += ",\trtz";
        return output;
    }

}
