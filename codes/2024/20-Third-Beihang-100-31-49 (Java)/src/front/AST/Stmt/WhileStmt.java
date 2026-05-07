package front.AST.Stmt;

import front.AST.Exp.Cond;
import front.AST.Node;
import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Value;
import utils.type.SyntaxType;

import java.util.ArrayList;

// WhileStmt ==> 'while' '(' Cond ')' Stmt
public class WhileStmt extends Stmt {
    public WhileStmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Value toIR() {
        symbolTableManager.enterCycle();
        Cond cond = null;
        Stmt stmt = null;
        for (Node child : children) {
            if (child instanceof Cond) {
                cond = ((Cond) child);
            } else if (child instanceof Stmt) {
                stmt = ((Stmt) child);
            }
        }

        //准备所需的子节点
        BasicBlock curBlock = irManager.getCurBlock();
        BasicBlock condBlock = new BasicBlock();
        //follow不是循环内部的块
        BasicBlock followBlock = new BasicBlock();
        BasicBlock stmtBlock = new BasicBlock();

        symbolTableManager.enterBlock();
        irManager.addBreakTo(followBlock);

        irManager.setCurBlock(curBlock);
        new Br(condBlock);

        irManager.setCurBlock(condBlock);
        assert cond != null;

        cond.condToIR(stmtBlock, followBlock);

        irManager.addContinueTo(condBlock);
        irManager.setCurBlock(stmtBlock);
        if (stmt != null) {
            stmt.toIR();
        }

        cond.condToIR(stmtBlock, followBlock);

        irManager.setCurBlock(followBlock);
        symbolTableManager.exitBlock();
        symbolTableManager.exitCycle();
        return null;

    }
}
