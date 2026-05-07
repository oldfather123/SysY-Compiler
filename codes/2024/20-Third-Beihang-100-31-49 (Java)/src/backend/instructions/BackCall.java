package backend.instructions;

import backend.operand.BackOperand;

public class BackCall extends BackInstruction {
    private String funcName;

    public BackCall(String name) {
        this.funcName = name;
    }
    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("\tcall").append(" ").append(funcName);
        return sb.toString();
    }
}
