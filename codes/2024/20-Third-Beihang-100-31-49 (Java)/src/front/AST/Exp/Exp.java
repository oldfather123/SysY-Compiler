package front.AST.Exp;

import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Value;
import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;
import java.lang.Number;

/*
 Exp ==> AddExp
 */
public class Exp extends Node {

    public Exp(SyntaxType SType, ArrayList<Node> children) {
        super(SType, children);
    }

    public Value toIR() {
        try {
            Number val = evaluate();
            return new ConstNumber(val);
        } catch (NullPointerException e) {
            return children.get(0).toIR();
        }
    }

    public Number evaluate() {
        return children.get(0).evaluate();
    }
}
