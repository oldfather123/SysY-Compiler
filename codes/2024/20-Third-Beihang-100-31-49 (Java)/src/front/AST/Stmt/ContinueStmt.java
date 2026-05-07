package front.AST.Stmt;

import front.AST.Node;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Value;
import utils.type.SyntaxType;

import java.util.ArrayList;

// ContinueStmt ==> 'continue' ';'
public class ContinueStmt extends Stmt {

    public ContinueStmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Value toIR() {
        return new Br(irManager.getContinueTo());
    }

}
