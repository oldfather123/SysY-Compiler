package front.AST.Exp;

import front.AST.Node;
import mid.IntermediatePresentation.BasicBlock;
import utils.type.SyntaxType;

import java.util.ArrayList;

/*
 Cond ==> LorExp
 */
public class Cond extends Node {

    public Cond(SyntaxType SType, ArrayList<Node> children) {
        super(SType, children);
    }

    public void condToIR(BasicBlock ifTrue, BasicBlock ifFalse) {
        ((LOrExp) children.get(0)).condToIR(ifTrue, ifFalse);
    }

}
