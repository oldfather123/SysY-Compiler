package front.AST.Stmt;

import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// Stmt ==> AssignStmt | ExpStmt | BlockStmt | IfStmt | WhileStmt | BreakStmt | ReturnStmt | GetIntStmt
public class Stmt extends Node {

    public Stmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

}
