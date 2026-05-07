package front.AST.Stmt;

import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// PutIntStmt ==> 'putint' '(' Exp ')' ';'
public class PutIntStmt extends ExpStmt {
    public PutIntStmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }
}
