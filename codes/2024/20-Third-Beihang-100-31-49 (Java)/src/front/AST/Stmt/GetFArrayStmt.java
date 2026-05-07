package front.AST.Stmt;

import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// GetFArrayStmt ==> LVal '=' 'getfarray' '(' Exp ')' ';'
public class GetFArrayStmt extends AssignStmt {

    public GetFArrayStmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }
}
