package front.AST.Func;

import mid.IntermediatePresentation.Function.Param;
import mid.IntermediatePresentation.Value;
import front.AST.Node;
import utils.type.SyntaxType;

import java.util.ArrayList;

// FuncFParams ==> FuncFParam { ',' FuncFParam }
public class FuncFParams extends Node {

    public FuncFParams(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Value toIR() {
        Param param = new Param();
        for (Node child : children) {
            if (child instanceof FuncFParam) {
                param.addParam(child.toIR());
            }
        }
        return param;
    }

}
