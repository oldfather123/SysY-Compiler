package frontend.AST;

import java.util.ArrayList;

//UnaryExp -> {UnaryOp} PrimaryExp
public class UnaryExp implements Exp {
    private ArrayList<String> operations;
    private PrimaryExp operand;

    public UnaryExp(ArrayList<String> operations, PrimaryExp operand) {
        this.operand = operand;
        this.operations = operations;
    }

    public ArrayList<String> getUnaryOps() {
        return operations;
    }

    public PrimaryExp getPrimaryExp() {
        return operand;
    }
}
