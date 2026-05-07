package front.AST.Stmt;

import front.AST.Node;
import mid.IntermediatePresentation.Value;
import mid.SymbolTable.SymbolTableManager;
import utils.type.SyntaxType;

import java.io.PipedOutputStream;
import java.util.ArrayList;

// BlockStmt ==> Block
public class BlockStmt extends Stmt {

    public BlockStmt(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Value toIR() {
        SymbolTableManager.getInstance().enterBlock();
        for (Node child : children) {
            child.toIR();
        }
        SymbolTableManager.getInstance().exitBlock();
        return null;
    }

}
