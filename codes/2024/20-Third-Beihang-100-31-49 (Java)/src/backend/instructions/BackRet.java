package backend.instructions;

import backend.operand.BackOperand;

public class BackRet extends BackInstruction{
    private BackOperand operand;
    public BackRet() {

    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("\tret");
        return sb.toString();
    }
}
