package front.AST.Var;

import mid.IntermediatePresentation.Value;
import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// ConstDecl ==> 'const' BType ConstDef { ',' ConstDef } ';'
public class ConstDecl extends Node {

    public ConstDecl(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Value toIR() {
        boolean isInt = ((BType) children.get(1)).isInt();

        for (Node child : children) {
            if (child instanceof ConstDef constDef) {
                constDef.toIR(isInt);
            }
        }
        return null;
    }

}
