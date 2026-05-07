package front.AST.Exp;

import front.AST.Node;
import mid.IntermediatePresentation.BasicBlock;
import utils.type.SyntaxType;

import java.util.ArrayList;

/*
 LOrExp ==> LAndExp { '||' LAndExp }
 */
public class LOrExp extends Node {

    public LOrExp(SyntaxType SType, ArrayList<Node> children) {
        super(SType, children);
    }

    public void condToIR(BasicBlock ifTrue, BasicBlock ifFalse) {
        if (children.size() == 1) {
            ((LAndExp) children.get(0)).condToIR(ifTrue, ifFalse);
        } else {
            for (Node child : children) {
                if (child instanceof LAndExp lAndExp) {
                    BasicBlock curBB = irManager.getCurBlock();
                    BasicBlock falseThen = child.equals(children.get(children.size() - 1)) ?
                            ifFalse : new BasicBlock();

                    irManager.setCurBlock(curBB);
                    lAndExp.condToIR(ifTrue, falseThen);
                    irManager.setCurBlock(falseThen);
                }
            }
        }
    }

}
