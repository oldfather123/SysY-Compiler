package front.AST.Stmt;

import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// PutFloatStmt ==> 'putfloat' '(' Exp ')' ';'
public class PutFloatStmt extends ExpStmt {
    public PutFloatStmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }
}
