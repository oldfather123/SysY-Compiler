package front.AST.Stmt;

import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// GetChStmt ==> LVal '=' 'getch' '(' ')' ';'
public class GetChStmt extends AssignStmt{
    public GetChStmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }
}
