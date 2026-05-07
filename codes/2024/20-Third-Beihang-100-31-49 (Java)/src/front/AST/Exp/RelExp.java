package front.AST.Exp;

import front.AST.Node;
import mid.IntermediatePresentation.Instruction.Cmp;
import mid.IntermediatePresentation.Value;
import utils.type.SyntaxType;

import java.util.ArrayList;

// RelExp ==> AddExp { ( '<' | '>' | '<=' | '>=' ) AddExp }
public class RelExp extends Node {

    public RelExp(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Value toIR() {
        if (children.size() == 1) {
            return children.get(0).toIR();
        } else {
            int pOp = 1;
            String op;
            Value val = children.get(0).toIR();
            while (pOp < children.size()) {
                op = children.get(pOp).getTokenValue();
                Value op1 = val;
                Value op2 = children.get(pOp + 1).toIR();
                boolean isInt = !(op1.isFloat() || op2.isFloat());
                val = new Cmp(op, op1.withType(isInt), op2.withType(isInt));
                pOp += 2;
            }
            return val;
        }
    }

}
