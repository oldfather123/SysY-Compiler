package front.AST.Stmt;

import front.AST.Exp.Cond;
import front.AST.Node;
import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.Value;
import utils.type.SyntaxType;

import java.util.ArrayList;

// IfStmt ==> 'if' '(' Cond ')' Stmt [ 'else' Stmt ]
public class IfStmt extends Stmt {
    public IfStmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Value toIR() {
        Cond cond = null;
        Stmt stmt1 = null, stmt2 = null;
        for (Node child : children) {
            if (child instanceof Cond) {
                cond = (Cond) child;
            } else if (child instanceof Stmt && stmt1 == null) {
                stmt1 = (Stmt) child;
            } else if (child instanceof Stmt) {
                stmt2 = (Stmt) child;
            }
        }
        if (stmt1 == null) {
            return null;
        }
        assert cond != null;
        //准备子表达式

        BasicBlock ifStmtBlock = irManager.getCurBlock();
        BasicBlock followBlock = new BasicBlock();
        irManager.setCurBlock(ifStmtBlock);
        //设置后继块

        BasicBlock ifTrue = new BasicBlock();
        if (stmt2 == null) {
            irManager.setCurBlock(ifStmtBlock);
            cond.condToIR(ifTrue, followBlock);
            irManager.setCurBlock(ifTrue);
            stmt1.toIRThenBrTo(followBlock);
        } else {
            BasicBlock ifFalse = new BasicBlock();

            irManager.setCurBlock(ifStmtBlock);
            cond.condToIR(ifTrue, ifFalse);
            irManager.setCurBlock(ifTrue);
            stmt1.toIRThenBrTo(followBlock);
            irManager.setCurBlock(ifFalse);
            stmt2.toIRThenBrTo(followBlock);
        }

        irManager.setCurBlock(followBlock);
        return null;
    }

}
