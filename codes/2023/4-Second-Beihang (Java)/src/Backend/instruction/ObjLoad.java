package Backend.instruction;

import Backend.operand.ObjFVirReg;
import Backend.operand.ObjOperand;

public class ObjLoad extends ObjInstr {
    private String type;
    private ObjOperand dst, addr, offset;

    public ObjLoad(ObjOperand dst, ObjOperand addr, ObjOperand offset, String ty) {
        type = ty;
        setDst(dst);
        setAddr(addr);
        setOffset(offset);
    }
    public ObjLoad(ObjOperand dst, ObjOperand addr, String ty) {
        type = ty;
        setDst(dst);
        setAddr(addr);
        isLa=true;
    }

    public void setDst(ObjOperand dst) {
        addDefReg(this.dst, dst);
        this.dst = dst;
    }
    public void setAddr(ObjOperand addr) {
        addUseReg(this.addr, addr);
        this.addr = addr;
    }
    public void setOffset(ObjOperand offset) {
        addUseReg(this.offset, offset);
        this.offset = offset;
    }
    public boolean isLa=false;

    public ObjOperand getAddr() { return addr; }
    public ObjOperand getOffset() { return offset; }
    public ObjOperand getDst() { return dst; }

    @Override
    public void replaceReg(ObjOperand oldReg, ObjOperand newReg) {
        if (dst.equals(oldReg))
            setDst(newReg);
        if (addr.equals(oldReg))
            setAddr(newReg);
        if (offset!=null && offset.equals(oldReg))
            setOffset(newReg);
    }

    @Override
    public void replaceUseReg(ObjOperand oldReg, ObjOperand newReg) {
        if (addr.equals(oldReg))
            setAddr(newReg);
        if (offset!=null && offset.equals(oldReg))
            setOffset(newReg);
    }

    @Override
    public String toString() {
        String output;
        if(!isLa)
        {
           output = type + "\t" + dst + ",\t" + offset + "(" + addr + ")";
            if(dst instanceof ObjFVirReg)
                output = "f" + output;
        }
        else
        {
            assert type.equals("la") || type.equals("lla");
          output = type+"\t" + dst + ",\t"  + addr ;
            if(dst instanceof ObjFVirReg)
                output = "f" + output;
        }
        return output;
    }
}
