package front.AST.Exp;

import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// UnaryOp ==> '+' | '-' | '!'
public class UnaryOp extends Node {

    public UnaryOp(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

}
