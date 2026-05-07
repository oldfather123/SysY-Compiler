package front.AST.Stmt;

import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// StartTimeStmt ==> 'starttime' '(' ')' ';'
public class StartTimeStmt extends ExpStmt{
    public StartTimeStmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }
}
