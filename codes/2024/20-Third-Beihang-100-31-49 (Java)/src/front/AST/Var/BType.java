package front.AST.Var;

import front.AST.Node;
import front.AST.TokenNode;
import utils.type.SyntaxType;

import java.util.ArrayList;

// BType ==> 'int' | 'float'
public class BType extends Node {
    public BType(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public boolean isInt() {
        return ((TokenNode) children.get(0)).getToken().getValue().equals("int");
    }
}
