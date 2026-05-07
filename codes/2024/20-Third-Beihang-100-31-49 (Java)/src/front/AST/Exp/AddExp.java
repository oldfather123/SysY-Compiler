package front.AST.Exp;

import front.AST.Node;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Instruction.ALU;
import mid.IntermediatePresentation.Value;
import utils.type.SyntaxType;

import java.lang.Number;
import java.util.ArrayList;

/*
 AddExp ==> MulExp { ( '+' | '-' ) MulExp }
 */
public class AddExp extends Node {

    public AddExp(SyntaxType SType, ArrayList<Node> children) {
        super(SType, children);
    }

    public Value toIR() {
        try {
            Number val = evaluate();
            return new ConstNumber(val);
        } catch (NullPointerException ignored) {
        }
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
                val = new ALU(op1.withType(isInt), op, op2.withType(isInt), isInt);
                pOp += 2;
            }
            return val;
        }
    }

    public Number evaluate() {
        if (children.size() == 1) {
            return children.get(0).evaluate();
        } else {
            int pOp = 1;
            String op;
            Number val = children.get(0).evaluate();
            while (pOp < children.size()) {
                op = children.get(pOp).getTokenValue();
                Number op2 = children.get(pOp + 1).evaluate();
                if (val instanceof Float || op2 instanceof Float) {
                    if (op.equals("+")) {
                        val = val.floatValue() + op2.floatValue();
                    } else if (op.equals("-")) {
                        val = val.floatValue() - op2.floatValue();
                    }
                } else {
                    if (op.equals("+")) {
                        val = val.intValue() + op2.intValue();
                    } else if (op.equals("-")) {
                        val = val.intValue() - op2.intValue();
                    }
                }
                pOp += 2;
            }
            return val;
        }
    }
}
