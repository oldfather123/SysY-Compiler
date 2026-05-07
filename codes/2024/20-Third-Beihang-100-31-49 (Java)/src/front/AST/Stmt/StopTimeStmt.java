package front.AST.Stmt;

import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// StopTimeStmt ==> 'stoptime' '(' ')';
public class StopTimeStmt extends ExpStmt {
    public StopTimeStmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }
}
