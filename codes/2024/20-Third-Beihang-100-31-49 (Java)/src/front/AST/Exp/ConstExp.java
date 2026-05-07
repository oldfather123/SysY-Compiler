package front.AST.Exp;

import front.AST.Node;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Value;
import utils.type.SyntaxType;

import java.util.ArrayList;

/*
 ConstExp ==> AddExp
 */
public class ConstExp extends Node {

    public ConstExp(SyntaxType SType, ArrayList<Node> children) {
        super(SType, children);
    }

    public Value toIR() {
        try {
            java.lang.Number val = evaluate();
            return new ConstNumber(val);
        } catch (NullPointerException e) {
            return children.get(0).toIR();
        }
    }

    public Number evaluate() {
        return children.get(0).evaluate();
    }

}
