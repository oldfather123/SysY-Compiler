package backend.instructions;

import backend.operand.BackImm;
import backend.operand.BackImm12;
import backend.operand.BackOperand;

import java.util.HashSet;

public class BackBinary extends BackInstruction {
    private String name;
    private BackOperand dst;
    private BackOperand src1;
    private BackOperand src2;

    public BackBinary(String name, BackOperand dst, BackOperand src1, BackOperand src2) {
        this.name = name;
        this.dst = dst;
        this.src1 = src1;
        this.src2 = src2;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        if (name.equals("add")) {
            if (src2 instanceof BackImm12) {
                name = name + "i";
            }
        } else if (name.equals("addw")) {
            if (src2 instanceof BackImm12) {
                name = "addiw";
            }
        }
        sb.append("\t").append(name).append(" ").append(dst.toString()).append(", ").append(src1.toString()).append(", ").append(src2.toString());
        return sb.toString();
    }

    public BackOperand getDst() {
        return dst;
    }

    public HashSet<BackOperand> getOperands() {
        HashSet<BackOperand> operands = new HashSet<>();
        operands.add(src1);
        operands.add(src2);
        return operands;
    }

    public void replaceOperand(BackOperand origin, BackOperand newOperand) {
        if (dst.equals(origin)) {
            dst = newOperand;
        }
        if (src1.equals(origin)) {
            src1 = newOperand;
        }
        if (src2.equals(origin)) {
            src2 = newOperand;
        }
    }

    public String getName() {
        return name;
    }

    public BackOperand getSrc1() {
        return src1;
    }

    public BackOperand getSrc2() {
        return src2;
    }
}
