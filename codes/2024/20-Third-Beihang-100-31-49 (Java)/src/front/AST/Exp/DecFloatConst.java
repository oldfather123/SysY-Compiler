package front.AST.Exp;

import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

public class DecFloatConst extends NumberNode {
    public DecFloatConst(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Number evaluate() {
        return Float.parseFloat(getTokenValue());
    }
}
