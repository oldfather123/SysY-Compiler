package front.AST.Stmt;

import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// PutChStmt ==> 'putch' '(' Exp ')' ';'
public class PutChStmt extends ExpStmt {
    public PutChStmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }
}
