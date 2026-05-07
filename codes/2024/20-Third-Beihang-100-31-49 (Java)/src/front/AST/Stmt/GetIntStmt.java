package front.AST.Stmt;

import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// GetIntStmt ==> LVal '=' 'getint' '(' ')' ';'
public class GetIntStmt extends AssignStmt {

    public GetIntStmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

}
