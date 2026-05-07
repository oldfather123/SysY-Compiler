package front.AST.Func;

import mid.IntermediatePresentation.Function.MainFunction;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;
import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// MainFuncDef ==> 'int' 'main' '(' ')' Block
public class MainFuncDef extends Node {

    public MainFuncDef(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Value toIR() {
        MainFunction mainFunction = new MainFunction();
        symbolTableManager.funcDecl(ValueType.I32, "@main");
        symbolTableManager.enterBlock();
        super.toIR();
        symbolTableManager.funcDeclEnd();
        return mainFunction;
    }

}
