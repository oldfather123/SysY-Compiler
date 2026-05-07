package backend.instruction;

import backend.component.ObjInstr;
import backend.operand.*;

public class ObjMove extends ObjInstr {
    //move,li
    private ObjOperand src;
    private ObjOperand dst;
    public boolean needchange = false;

    public ObjMove(ObjOperand src, ObjOperand dst) {
        super("move");
        this.src = src;
        this.dst = dst;
        addDefReg(this.dst, dst);
        addUseReg(this.src, src);
        if (src instanceof ObjFVirReg || src instanceof ObjFPhyReg ||
                dst instanceof ObjFVirReg || dst instanceof ObjFPhyReg) {
            changeType("fmv.s");
        } else if (src instanceof ObjImm) {
            changeType("li");
        } else if (src instanceof ObjLabel) {
            changeType("la");
        }
    }

    public ObjMove(String type, ObjOperand src, ObjOperand dst) {
        super(type);
        this.src = src;
        this.dst = dst;
        addDefReg(this.dst, dst);
        addUseReg(this.src, src);
    }

    public ObjOperand getSrc() {
        return src;
    }

    public ObjOperand getDst() {
        return dst;
    }

    public void setDst(ObjOperand dst) {
        addDefReg(this.dst, dst);
        this.dst = dst;
    }

    public void setSrc(ObjOperand src) {
        addUseReg(this.src, src);
        this.src = src;
    }

    @Override
    public String toString() {
        if (src instanceof ObjImm) {
            return "li\t" + dst + ",\t" + src;
        } else if (src instanceof ObjLabel) {
            return "la\t" + dst + ",\t" + src;
        }  else if (src instanceof ObjFPhyReg || src instanceof ObjFVirReg) {
            if(dst instanceof ObjFPhyReg || dst instanceof ObjFVirReg){
                return "fmv.s" + "\t" + dst + ",\t" + src;
            }else{
                return "fmv.x.w\t" + dst + ",\t" + src;
            }
        }
        else if (dst instanceof ObjFPhyReg || dst instanceof ObjFVirReg) {
            return "fmv.w.x\t" + dst + ",\t" + src;
        } else {
            return "mv\t" + dst + ",\t" + src;
        }
    }

    @Override
    public void replaceReg(ObjOperand oldReg, ObjOperand newReg) {
        if (dst.equals(oldReg))
            setDst(newReg);
        if (src.equals(oldReg))
            setSrc(newReg);
    }

    public void appendOffsetIfMarked(int append) {
        if (this.getType().equals("li.stack")) {
            this.src = new ObjImm(append + ((ObjImm) src).getImmediate());
        }
        this.setType("li");
    }
}
