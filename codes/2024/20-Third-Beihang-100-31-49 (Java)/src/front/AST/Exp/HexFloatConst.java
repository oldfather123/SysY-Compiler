package front.AST.Exp;

import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

public class HexFloatConst extends NumberNode {
    public HexFloatConst(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Number evaluate() {
        return Float.parseFloat(getTokenValue());
    }
}
