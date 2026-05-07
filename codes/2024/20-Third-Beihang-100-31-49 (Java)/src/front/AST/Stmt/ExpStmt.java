package front.AST.Stmt;

import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// ExpStmt ==> [ Exp ] ';'
public class ExpStmt extends Stmt {
    public ExpStmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

}
