package front.AST.Stmt;

import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// GetArrayStmt ==> LVal '=' 'getarray' '(' Exp ')' ';'
public class GetArrayStmt extends AssignStmt {
    public GetArrayStmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }
}