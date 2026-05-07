package backend.instruction;

import backend.component.ObjInstr;
import backend.operand.ObjImm;
import backend.operand.ObjOperand;

public class ObjBinary extends ObjInstr {
    //add,addi,sub,mul,div,
    //fadd.s,fsub.s,fmul.s,fdiv.s,
    //and,andi,or,ori,xor,xori,sll,slli,srl,srli,sra,srai
    //rem,sltu
    //TODO:确保没有立即数的指令不会传入立即数操作数
    private String opType;//操作数类型
    private ObjOperand dst;
    private ObjOperand src1;
    private ObjOperand src2;
    private boolean canBeAlter = false;

    public ObjBinary(String type, String opType, ObjOperand dst, ObjOperand src1, ObjOperand src2) {
        super(type);//可能不完善
        this.opType = opType;
        this.dst = dst;
        if (src1 instanceof ObjImm && !(src2 instanceof ObjImm)) {
            //交换立即数位置
            ObjOperand tmp = src1;
            src1 = src2;
            src2 = tmp;
        }
        if (src2 instanceof ObjImm) {
            if (type.equals("sub")) {
                super.changeType("add");
                src2 = new ObjImm(-((ObjImm) src2).getImmediate());
            }
        }
        this.src1 = src1;
        this.src2 = src2;
        addDefReg(this.dst, dst);
        addUseReg(this.src1, src1);
        addUseReg(this.src2, src2);
    }

    public ObjOperand getDst() {
        return dst;
    }

    public ObjOperand getSrc1() {
        return src1;
    }

    public ObjOperand getSrc2() {
        return src2;
    }

    public void setDst(ObjOperand dst) {
        addDefReg(this.dst, dst);
        this.dst = dst;
    }
    public void setSrc1(ObjOperand src1) {
        addUseReg(this.src1, src1);
        this.src1 = src1;
    }
    public void setSrc2(ObjOperand src2) {
        addUseReg(this.src2, src2);
        this.src2 = src2;
    }

    public void setCanBeAlter(boolean canBeAlter) {
        this.canBeAlter = canBeAlter;
    }

    public void changeSrc2(int immediate) {
        this.src2 = new ObjImm(immediate);
    }

    @Override
    public String toString() {
        String string = getType();
        //有立即数的指令
        if (src2 instanceof ObjImm) {
            if (getType().equals("add") || getType().equals("xor") || getType().equals("or") || getType().equals("and")
                    || getType().equals("sll") || getType().equals("srl") || getType().equals("sra")) {
                string += "i";
            } else if (getType().equals("sltu")) {
                string = "sltiu";
            } else if (getType().equals("addw")) {
                string = "addiw";
            }
        }
        string += "\t" + dst + ",\t" + src1 + ",\t" + src2;
        return string;
    }

    @Override
    public void replaceReg(ObjOperand oldReg, ObjOperand newReg) {
        if (dst.equals(oldReg))
            setDst(newReg);
        if (src1.equals(oldReg))
            setSrc1(newReg);
        if (src2.equals(oldReg))
            setSrc2(newReg);
    }
}
