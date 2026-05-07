package backend.instructions;

import backend.operand.BackOperand;
import backend.tools.BackBlock;

import java.util.HashSet;

public class BackBranch extends BackInstruction {
    private String label;
    private String name;
    private BackOperand cond;
    private final BackBlock dst;

    public BackBranch(BackBlock block) {
        this.label = block.getName();
        this.dst = block;
    }

    public BackBranch(String name, BackOperand cond, BackBlock block) {
        this.name = name;
        this.cond = cond;
        this.dst = block;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        if (cond != null) {
            sb.append("\t").append(name).append(" ").append(cond).append(", ").append(dst.getName());
        } else {
            sb.append("\tj").append(" ").append(label);
        }
        return sb.toString();
    }

    public HashSet<BackOperand> getOperands() {
        HashSet<BackOperand> operands = new HashSet<>();
        if (cond != null) {
            operands.add(cond);
        }
        return operands;
    }

    public BackBlock getDstBlock() {
        return dst;
    }

    public void replaceOperand(BackOperand origin, BackOperand newOperand) {
        if (cond.equals(origin)) {
            cond = newOperand;
        }
    }
}
