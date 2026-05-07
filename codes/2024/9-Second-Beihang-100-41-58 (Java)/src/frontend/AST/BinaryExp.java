package frontend.AST;

import java.util.ArrayList;

//BinaryExp -> Exp {BinaryOp Exp}
//BinaryOp -> + - > >= < <= == && != ||
public class BinaryExp implements Exp {
    private Exp first;
    private ArrayList<String> binaryOps;
    private ArrayList<Exp> exps;

    public BinaryExp(Exp first, ArrayList<String> binaryOps, ArrayList<Exp> exps) {
        this.first = first;
        this.binaryOps = binaryOps;
        this.exps = exps;
    }

    public Exp getFirst() {
        return this.first;
    }

    public ArrayList<String> getBinaryOps() {
        return this.binaryOps;
    }

    public ArrayList<Exp> getExps() {
        return this.exps;
    }
}
