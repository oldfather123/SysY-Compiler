package front.AST.Exp;

import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

public class OctConst extends NumberNode {
    public OctConst(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Number evaluate() {
        return Integer.parseInt(getTokenValue().substring(1), 8);
    }
}
