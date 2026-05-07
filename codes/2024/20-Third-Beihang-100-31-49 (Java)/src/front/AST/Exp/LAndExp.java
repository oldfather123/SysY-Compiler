package front.AST.Exp;

import front.AST.Node;
import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Instruction.ZextTo;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;
import utils.type.SyntaxType;

import java.util.ArrayList;

/*
 LAndExp ==> EqExp { '&&' EqExp }
 */
public class LAndExp extends Node {

    public LAndExp(SyntaxType SType, ArrayList<Node> children) {
        super(SType, children);
    }

    public void condToIR(BasicBlock ifTrue, BasicBlock ifFalse) {
        Value cond;

        if (children.size() == 1) {
            cond = children.get(0).toIR();
            new Br(cond.toI1(), ifTrue, ifFalse);
        } else {
            for (Node child : children) {
                if (child instanceof EqExp eqExp) {
                    BasicBlock curBB = irManager.getCurBlock();
                    BasicBlock trueThen = child.equals(children.get(children.size() - 1)) ?
                            ifTrue : new BasicBlock();
                    irManager.setCurBlock(curBB);

                    cond = eqExp.toIR();
                    new Br(cond.toI1(), trueThen, ifFalse);
                    irManager.setCurBlock(trueThen);
                }
            }
        }
    }
}
