package front.AST.Stmt;

import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// GetFloatStmt ==> LVal '=' 'getfloat' '(' ')' ';'
public class GetFloatStmt extends AssignStmt {

    public GetFloatStmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }
}
