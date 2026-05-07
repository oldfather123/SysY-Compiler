package front.AST.Stmt;

import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// PutArrayStmt ==> 'putarray' '(' Exp ',' Exp { ',' Exp} ')' ';'
public class PutArrayStmt extends ExpStmt {
    public PutArrayStmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }
}
