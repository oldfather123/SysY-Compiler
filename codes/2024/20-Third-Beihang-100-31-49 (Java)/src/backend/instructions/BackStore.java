package backend.instructions;

import backend.operand.BackIReg;
import backend.operand.BackImm;
import backend.operand.BackImm12;
import backend.operand.BackOperand;

import java.util.HashSet;

public class BackStore extends BackInstruction {
    private String name;
    private BackOperand src;
    private BackOperand dst;
    private BackOperand off;

    public BackStore(String name, BackOperand src, BackOperand off, BackOperand dst) {
        this.name = name;
        this.src = src;
        this.dst = dst;
        this.off = off;
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("\t").append(name).append(" ").append(src).append(", ").append(off).append("(").append(dst).append(")");
        return sb.toString();
    }

    public HashSet<BackOperand> getOperands() {
        HashSet<BackOperand> operands = new HashSet<>();
        operands.add(src);
        operands.add(off);
        operands.add(dst);
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

    public Integer getOffset() {
        if (off instanceof BackImm backImm) {
            return backImm.getImm();
        } else if (off instanceof BackImm12 backImm12) {
            return backImm12.getImm();
        } else {
            return null;
        }
    }

    public BackOperand getSrc() {
        return src;
    }

    public BackIReg getBase() {
        return (BackIReg) dst;
    }

    public String getName() {
        return name;
    }
}
