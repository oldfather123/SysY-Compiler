package backend.instruction;

import backend.component.ObjInstr;
import backend.operand.ObjOperand;

public class ObjLoad extends ObjInstr {
    private ObjOperand dst;
    private ObjOperand addr;
    private ObjOperand offset;

    public ObjLoad(String type, ObjOperand dst, ObjOperand addr, ObjOperand offset) {
        super(type);
        this.dst = dst;
        this.addr = addr;
        this.offset = offset;
        addUseReg(this.addr, addr);
        addUseReg(this.offset, offset);
        addDefReg(this.dst, dst);
    }

    public ObjOperand getDst() {
        return dst;
    }

    public void setAddr(ObjOperand addr) {
        addUseReg(this.addr, addr);
        this.addr = addr;
    }

    public void setDst(ObjOperand dst) {
        addDefReg(this.dst, dst);
        this.dst = dst;
    }

    public void setOffset(ObjOperand offset) {
        addUseReg(this.offset, offset);
        this.offset = offset;
    }

    public int getOffset() {
        // offset shall be an imm ?
        return ((backend.operand.ObjImm) offset).getImmediate();
    }

    @Override
    public String toString() {
        String string = getType();
        if (getType().equals("la") || getType().equals("lla")) {
            string += "\t" + dst + ",\t" + addr;
            //if(dst instanceof ObjFVirReg)?
        } else {
            string += "\t" + dst + ",\t" + offset + "(" + addr + ")";
        }
        return string;
    }

    @Override
    public void replaceReg(ObjOperand oldReg, ObjOperand newReg) {
        if (dst.equals(oldReg))
            setDst(newReg);
        if (addr.equals(oldReg))
            setAddr(newReg);
        if (offset!=null && offset.equals(oldReg))
            setOffset(newReg);
    }

    public ObjOperand getAddr() {
        return addr;
    }
}
