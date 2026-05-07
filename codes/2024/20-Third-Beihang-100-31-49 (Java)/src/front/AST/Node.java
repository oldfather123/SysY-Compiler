package front.AST;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.Function.LibFunction;
import mid.IntermediatePresentation.Instruction.Br;
import mid.SymbolTable.SymbolTableManager;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Value;
import utils.type.SyntaxType;

import java.util.ArrayList;

public class Node {

    public ArrayList<Node> children;
    public SyntaxType sType;

    protected SymbolTableManager symbolTableManager = SymbolTableManager.getInstance();
    protected IRManager irManager = IRManager.getInstance();

    public Node(SyntaxType sType, ArrayList<Node> children) {
        this.sType = sType;
        this.children = children;
    }

    public ArrayList<Node> getChildren() {
        return children;
    }

    public int getDim() {
        return -2;
    }

    public Number evaluate() {
        // 内部的求值一律使用浮点数
        return 0.0f;
    }

    // recursive checkError
    public void checkError() {
        // be careful to null
        if (children == null) {
            return;
        }
        for (Node child : children) {
            child.checkError();
        }
    }

    // recursive genIR
    public Value toIR() {
        // be careful to null
        if (children == null) {
            return null;
        }
        for (Node child : children) {
            child.toIR();
        }
        return null;
    }

    public String getTokenValue() {
        Node child = this;
        while (!(child instanceof TokenNode)) {
            child = child.children.get(0);
        }
        return ((TokenNode) child).getToken().getValue();
    }

    public void toIRThenBrTo(BasicBlock block) {
        toIR();
        new Br(block);
    }
}
