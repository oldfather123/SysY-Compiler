package backend.instructions;

import backend.operand.BackOperand;

import java.util.HashSet;

public class BackInstruction {
    public BackOperand getDst() {
        return null;
    }

    public HashSet<BackOperand> getOperands() {
        return new HashSet<>();
    }

    public void replaceOperand(BackOperand origin,BackOperand newOperand) {

    }
}
