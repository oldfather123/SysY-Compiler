package front.AST.Var;

import mid.IntermediatePresentation.Value;
import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// VarDecl ==> BType VarDef { ',' VarDef } ';'
public class VarDecl extends Node {

    public VarDecl(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Value toIR() {
        boolean isInt = ((BType) children.get(0)).isInt();

        for (Node child : children) {
            if (child instanceof VarDef varDef) {
                varDef.toIR(isInt);
            }
        }
        return null;
    }

}
