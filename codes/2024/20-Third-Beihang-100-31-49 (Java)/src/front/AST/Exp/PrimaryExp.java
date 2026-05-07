package front.AST.Exp;

import mid.IntermediatePresentation.Value;
import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// PrimaryExp ==> '(' Exp ')' | LVal | Number
public class PrimaryExp extends Node {

    public PrimaryExp(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Number evaluate() {
        if (children.get(0) instanceof NumberNode numberNode) {
            return numberNode.evaluate();
        } else if (children.get(0) instanceof LVal) {
            return children.get(0).evaluate();
        } else {
            return children.get(1).evaluate();
        }
    }

    public Value toIR() {
        if (children.size() == 1) {
            //Number or Lval
            return children.get(0).toIR();
        } else {
            //'(' Exp ')'
            return children.get(1).toIR();
        }
    }
}
