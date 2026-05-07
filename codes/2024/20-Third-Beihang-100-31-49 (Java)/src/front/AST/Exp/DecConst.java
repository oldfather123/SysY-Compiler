package front.AST.Exp;

import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

public class DecConst extends NumberNode {
    public DecConst(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Number evaluate() {
        return Integer.parseInt(getTokenValue());
    }

}
