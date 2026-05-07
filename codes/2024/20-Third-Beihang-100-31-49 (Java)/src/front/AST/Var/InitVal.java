package front.AST.Var;

import mid.IntermediatePresentation.Array.ArrayInitializer;
import mid.IntermediatePresentation.Value;
import front.AST.Node;
import mid.SymbolTable.SymbolTableManager;
import utils.type.SyntaxType;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.Objects;

// InitVal ==> Exp | '{' [ InitVal { ',' InitVal } ] '}'
public class InitVal extends Node {

    public InitVal(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public Number evaluate() {
        if (children.size() == 1) {
            return children.get(0).evaluate();
        } else {
            return null;
        }
    }

    public Value toIR(String ident, int curdim) {
        /*
            1. exp -> idx++
            2. ainit -> idx+=flattenLen/dimLen[curdim]
         */
        if (children.size() == 1) {
            return children.get(0).toIR();
        } else if (children.size() == 2) {
            ArrayList<Integer> dimLens = SymbolTableManager.getInstance().getDimLens(ident);
            dimLens = new ArrayList<>(dimLens.subList(curdim, dimLens.size()));
            int flattenLen = 1;
            for (Integer dimLen : dimLens) {
                flattenLen *= dimLen;
            }
            return new ArrayInitializer(new ArrayList<>(), flattenLen);
        } else {
            ArrayList<Integer> dimLens = SymbolTableManager.getInstance().getDimLens(ident);
            dimLens = new ArrayList<>(dimLens.subList(curdim, dimLens.size()));
            int flattenLen = 1;
            for (Integer dimLen : dimLens) {
                flattenLen *= dimLen;
            }
            ArrayInitializer initializer = new ArrayInitializer(new ArrayList<>(), flattenLen);
            int index = 0;
            int lenInBrace = flattenLen;
            if (curdim < dimLens.size()) {
                lenInBrace /= dimLens.get(curdim);
            }
            for (Node child : children) {
                if (child instanceof InitVal initVal) {
                    Value v = initVal.toIR(ident, curdim + 1);
                    if (v instanceof ArrayInitializer aiv) {
                        initializer.merge(aiv, index);
                        index += lenInBrace;
                    } else {
                        initializer.add(index, v);
                        index++;
                    }
                }
            }
            return initializer;
        }
    }

    public boolean isEmpty() {
        return children.size() == 2;
    }

}
