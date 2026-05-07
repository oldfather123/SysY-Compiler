package front.AST.Stmt;

import front.AST.Exp.Exp;
import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// PutFArrayStmt ==> 'putfarray' '(' Exp ',' Exp { ',' Exp } ')' ';'
public class PutFArrayStmt extends ExpStmt {
    public PutFArrayStmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }
}