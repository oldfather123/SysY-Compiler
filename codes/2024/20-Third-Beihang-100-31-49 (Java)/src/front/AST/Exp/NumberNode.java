package front.AST.Exp;

import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Value;
import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// Number ==> HEXCON,
//    OCTCON,
//    DECCON,
//    HEXFCON,
//    DECFCON
public class NumberNode extends Node {

    public NumberNode(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Number evaluate() {
        return children.get(0).evaluate();
    }

    public Value toIR() {
        return new ConstNumber(evaluate());
    }

}
