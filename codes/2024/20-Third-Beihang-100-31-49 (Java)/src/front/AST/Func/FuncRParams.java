package front.AST.Func;

import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// FuncRParams ==> Exp { ',' Exp }
public class FuncRParams extends Node {

    public FuncRParams(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

}
