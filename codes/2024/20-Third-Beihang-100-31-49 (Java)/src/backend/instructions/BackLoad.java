package backend.instructions;

import backend.operand.BackIReg;
import backend.operand.BackImm;
import backend.operand.BackImm12;
import backend.operand.BackOperand;

import java.util.HashSet;

public class BackLoad extends BackInstruction {
    private String name;
    private BackOperand src;
    private BackOperand dst;
    private BackOperand off;

    public BackLoad(String name, BackOperand dst, BackOperand off, BackOperand src) {
        this.name = name;
        this.src = src;
        this.dst = dst;
        this.off = off;
    }

    public BackLoad(String name, BackOperand dst, BackOperand src) {
        this.name = name;
        this.src = src;
        this.dst = dst;
    }

    public BackOperand getOff() {
        return this.off;
    }

    public void changeOffset(BackOperand off) {
        this.off = off;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        if (this.off != null) {
            sb.append("\t").append(name).append(" ").append(dst).append(", ").append(off).append("(").append(src).append(")");
        } else {
            sb.append("\t").append(name).append(" ").append(dst).append(", ").append(src);
        }
        return sb.toString();
    }

    public BackOperand getDst() {
        return dst;
    }

    public BackIReg getBase() {
        return (BackIReg) src;
    }

    public Integer getOffset() {
        if (off instanceof BackImm12 backImm12) {
            return backImm12.getImm();
        } else if (off instanceof BackImm backImm) {
            return backImm.getImm();
        } else {
            return null;
        }
    }

    public HashSet<BackOperand> getOperands() {
        HashSet<BackOperand> operands = new HashSet<>();
        operands.add(src);
        operands.add(off);
        return operands;
    }

    public void replaceOperand(BackOperand origin, BackOperand newOperand) {
        if (dst.equals(origin)) {
            dst = newOperand;
        }
        if (src.equals(origin)) {
            src = newOperand;
        }
        if (off.equals(origin)) {
            off = newOperand;
        }
    }

    public String getName(){
        return name;
    }
}
