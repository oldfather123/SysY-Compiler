package front.AST.Func;

import mid.IntermediatePresentation.ValueType;
import front.AST.Node;
import front.AST.TokenNode;
import utils.type.SyntaxType;

import java.util.ArrayList;

// FuncType ==> 'void' | 'int' | 'float'
public class FuncType extends Node {

    public FuncType(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public ValueType getRetType() {
        String retType = ((TokenNode) children.get(0)).getToken().getValue();
        if (retType.equals("void")) {
            return ValueType.NULL;
        } else if (retType.equals("int")) {
            return ValueType.I32;
        } else {
            return ValueType.FLT;
        }
    }

}
