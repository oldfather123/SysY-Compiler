package backend.instructions;

import backend.operand.BackImm;
import backend.operand.BackLabel;
import backend.operand.BackOperand;

import java.util.HashSet;

public class BackMov extends BackInstruction {
    private String name;
    private BackOperand dst;
    private BackOperand src;

    public BackMov(BackOperand dst, BackOperand src) {
        if (src.toString().charAt(0) == 'f') { //stupid
            this.name = "fmv.s";
        } else if (src instanceof BackLabel) {
            this.name = "la";
        } else {
            if (src instanceof BackImm) {
                this.name = "li";
            } else {
                this.name = "move";
            }
        }
        this.dst = dst;
        this.src = src;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        if (!dst.toString().equals(src.toString())) {
            sb.append("\t").append(name).append(" ").append(dst.toString()).append(", ").append(src.toString());
        }
        return sb.toString();
    }

    public BackOperand getDst() {
        return dst;
    }

    public HashSet<BackOperand> getOperands() {
        HashSet<BackOperand> operands = new HashSet<>();
        operands.add(src);
        return operands;
    }

    public BackOperand getSrc() {
        return src;
    }

    public void replaceOperand(BackOperand origin, BackOperand newOperand) {
        if (dst.equals(origin)) {
            dst = newOperand;
        }
        if (src.equals(origin)) {
            src = newOperand;
        }
    }
}
