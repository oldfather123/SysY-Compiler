package backend.instructions;

import backend.operand.BackOperand;

import java.util.HashSet;

public class BackConversion extends BackInstruction {
    private Integer direction;
    private BackOperand dst;
    private BackOperand src;

    public BackConversion(BackOperand dst, BackOperand src, Integer direction) {
        this.direction = direction;
        this.dst = dst;
        this.src = src;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        if (direction == 0) { //int -> float
            sb.append("\tfcvt.s.w ").append(dst.toString()).append(", ").append(src.toString());
        } else {
            sb.append("\tfcvt.w.s ").append(dst.toString()).append(", ").append(src.toString()).append(", rtz");
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

    public void replaceOperand(BackOperand origin, BackOperand newOperand) {
        if (dst.equals(origin)) {
            dst = newOperand;
        }
        if (src.equals(origin)) {
            src = newOperand;
        }
    }
}
