package Backend.instruction;

import Backend.operand.ObjFVirReg;
import Backend.operand.ObjOperand;

public class ObjStore extends ObjInstr {
    private String type;
    private ObjOperand src, addr, offset;

    public ObjStore(ObjOperand src, ObjOperand addr, ObjOperand offset, String ty) {
        type = ty;
        setSrc(src);
        setAddr(addr);
        setOffset(offset);
    }
    public void setSrc(ObjOperand src) {
        addUseReg(this.src, src);
        this.src = src;
    }
    public void setAddr(ObjOperand addr) {
        addUseReg(this.addr, addr);
        this.addr = addr;
    }
    public void setOffset(ObjOperand offset) {
        addUseReg(this.offset, offset);
        this.offset = offset;
    }

    public ObjOperand getAddr() { return addr; }
    public ObjOperand getOffset() { return offset; }
    public ObjOperand getSrc() { return src; }

    @Override
    public void replaceReg(ObjOperand oldReg, ObjOperand newReg) {
        if (src.equals(oldReg))
            setSrc(newReg);
        if (addr.equals(oldReg))
            setAddr(newReg);
        if (offset.equals(oldReg))
            setOffset(newReg);
    }
    @Override
    public void replaceUseReg(ObjOperand oldReg, ObjOperand newReg) {
        if (src.equals(oldReg))
            setSrc(newReg);
        if (addr.equals(oldReg))
            setAddr(newReg);
        if (offset.equals(oldReg))
            setOffset(newReg);
    }

    @Override
    public String toString() {
        String output = type + "\t" + src + ",\t" + offset + "(" + addr + ")";
        if(src instanceof ObjFVirReg)
            output = "f" + output;
        return output;
    }
}
