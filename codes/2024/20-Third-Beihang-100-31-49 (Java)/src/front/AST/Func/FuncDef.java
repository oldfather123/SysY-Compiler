package front.AST.Func;

import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.Function.Param;
import mid.IntermediatePresentation.Instruction.Ret;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;
import front.AST.Block;
import front.AST.Node;
import front.AST.TokenNode;
import utils.type.SyntaxType;

import java.util.ArrayList;

// FuncDef ==> FuncType Ident '(' [ FuncFParams ] ')' Block
public class FuncDef extends Node {
    public FuncDef(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Value toIR() {
        ValueType funcType = null;
        String ident = ((TokenNode) children.get(1)).getToken().getValue();
        FuncFParams funcFParams = null;
        Block block = null;

        for (Node child : children) {
            if (child instanceof FuncType ft) {
                funcType = ft.getRetType();
            } else if (child instanceof FuncFParams) {
                funcFParams = ((FuncFParams) child);
            } else if (child instanceof Block) {
                block = ((Block) child);
            }
        }

        symbolTableManager.funcDecl(funcType, "@" + ident);
        symbolTableManager.enterBlock();
        Function f;
        if (funcFParams == null) {
            f = new Function("@" + ident, new Param(), funcType);
        } else {
            f = new Function("@" + ident, funcType);
            f.setParam((Param) funcFParams.toIR());
        }

        symbolTableManager.setIRValue("@" + ident, f);
        assert block != null;
        block.toIR();

        if (f.getType() == ValueType.NULL) {
            new Ret();
        }
        symbolTableManager.funcDeclEnd();
        return f;
    }
}
