package front.AST.Exp;

import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Instruction.ALU;
import mid.IntermediatePresentation.Instruction.GetElementPtr;
import mid.IntermediatePresentation.Value;
import front.AST.Node;
import mid.IntermediatePresentation.ValueType;
import utils.type.SyntaxType;

import java.util.ArrayList;

/*
 LVal ==> Ident { '[' Exp ']' }
 */
public class LVal extends Node {

    public LVal(SyntaxType SType, ArrayList<Node> children) {
        super(SType, children);
    }

    public Number evaluate() {
        ArrayList<Exp> indexs = new ArrayList<>();
        for (Node child : children) {
            if (child instanceof Exp exp) {
                indexs.add(exp);
            }
        }

        String ident = getTokenValue();
        if (indexs.size() == 0) {
            return symbolTableManager.getVal(ident);
        } else {
            try {
                int p = indexs.size() - 1;
                int flattenIndex = 0;
                ArrayList<Integer> dimLens = symbolTableManager.getDimLens(ident);
                int fullFalttenIndex = 1;
                for (Integer dimLen : dimLens) {
                    fullFalttenIndex *= dimLen;
                }
                for (int i = 0; i <= p; i++) {
                    fullFalttenIndex /= dimLens.get(i);
                    flattenIndex += indexs.get(i).evaluate().intValue() * fullFalttenIndex;
                }
                return symbolTableManager.getArrayVal(ident, flattenIndex);
            } catch (NullPointerException e) {
                return null;
            }
        }
    }

    public Value toIR() {
        try {
            return new ConstNumber(evaluate());
        } catch (NullPointerException ignored) {
        }

        String ident = getTokenValue();
        Value v = symbolTableManager.getIRValue(ident);
        ArrayList<Exp> indexs = new ArrayList<>();
        for (Node child : children) {
            if (child instanceof Exp exp) {
                indexs.add(exp);
            }
        }

        int lvalDim = symbolTableManager.getDim(ident);
        if (indexs.size() == 0) {
            if (lvalDim != 0) {
                //已经都转为一维数组了，且没有取值操作，不需要区分维数
                return new GetElementPtr(v, (new ConstNumber(0)).withType(true));
            } else {
                return v;
            }
        } else {
            int fullFalttenIndex = 1;
            int p = indexs.size() - 1;
            Value flattenIndex = (new ConstNumber(0)).withType(true);
            ArrayList<Integer> dimLens = symbolTableManager.getDimLens(ident);
            for (Integer dimLen : dimLens) {
                fullFalttenIndex *= dimLen;
            }
            for (int i = 0; i <= p; i++) {
                fullFalttenIndex /= dimLens.get(i);

                Value op1 = new ConstNumber(fullFalttenIndex);
                Value op2 = indexs.get(i).toIR();
                boolean isInt = !(op1.isFloat() || op2.isFloat());
                Value offset = new ALU(op1.withType(isInt), "*", op2.withType(isInt), isInt);

                op1 = flattenIndex;
                op2 = offset;
                isInt = !(op1.isFloat() || op2.isFloat());
                flattenIndex = new ALU(flattenIndex.withType(isInt), "+",
                        offset.withType(isInt), isInt);
            }
            return new GetElementPtr(v, flattenIndex.withType(true));
        }
    }
}
