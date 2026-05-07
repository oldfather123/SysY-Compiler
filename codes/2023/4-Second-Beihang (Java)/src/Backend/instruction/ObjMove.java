package Backend.instruction;

import Backend.component.ObjGlobalVariable;
import Backend.operand.*;

public class ObjMove extends ObjInstr{
    private ObjOperand dst, src;
    public boolean needchange=false;
    public ObjMove(ObjOperand dst, ObjOperand src) {
        setDst(dst);
        setSrc(src);
    }

    public void setDst(ObjOperand dst) {
        addDefReg(this.dst, dst);
        this.dst = dst;
    }

    public void setSrc(ObjOperand src) {
        addUseReg(this.src, src);
        this.src = src;
    }
    public ObjOperand getDst() {
        return dst;
    }
    public ObjOperand getSrc() {
        return src;
    }

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

    @Override
    public String toString() {

        if(src instanceof ObjFPhyReg || src instanceof ObjFVirReg)
        {
            return "fmv.s\t" + dst + ",\t" + src;
        }

        if(src instanceof ObjLabel)
            return "la\t" + dst + ",\t" + src;
        else if ((src instanceof ObjImm) || (src instanceof ObjImm12)) {
            return "li\t" + dst + ",\t" + src;
        }
        return "move\t" + dst + ",\t" + src;
    }
}
