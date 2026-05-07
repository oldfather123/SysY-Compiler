package front.AST.Var;

import front.AST.Exp.ConstExp;
import front.AST.Node;
import front.AST.TokenNode;
import mid.IntermediatePresentation.Array.ArrayInitializer;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.Alloca;
import mid.IntermediatePresentation.Instruction.Call;
import mid.IntermediatePresentation.Instruction.GetElementPtr;
import mid.IntermediatePresentation.Instruction.GlobalDecl;
import mid.IntermediatePresentation.Instruction.Store;
import mid.IntermediatePresentation.Value;
import mid.SymbolTable.SymbolTableManager;
import utils.type.SyntaxType;

import java.util.ArrayList;
import java.util.HashMap;

// ConstDef ==> Ident { '[' ConstExp ']' } '=' ConstInitVal
public class ConstDef extends Node {

    public ConstDef(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public void toIR(boolean isInt) {
        String ident = ((TokenNode) children.get(0)).getToken().getValue();
        int dim = 0;
        ArrayList<Integer> dimLens = new ArrayList<>();
        ConstInitVal constInitVal = null;

        for (Node child : children) {
            if (child instanceof ConstExp constExp) {
                dim++;
                dimLens.add(constExp.evaluate().intValue());
            }
            if (child instanceof ConstInitVal) {
                constInitVal = ((ConstInitVal) child);
            }
        }
        assert constInitVal != null;

        symbolTableManager.varDecl(ident, true, dim, dimLens);
        int flattenLen = 1;
        for (Integer len : dimLens) {
            flattenLen *= len;
        }


        if (dim == 0) {
            Number val = constInitVal.evaluate();
            if (isInt) {
                val = val.intValue();
            } else {
                val = val.floatValue();
            }
            symbolTableManager.setVal(ident, val);

            //可以求出常数形式的初始值
            if (irManager.inGlobalDecl()) {
                GlobalDecl globalDecl = new GlobalDecl(
                        (new ConstNumber(val)).withType(isInt), isInt, true);

                symbolTableManager.setIRValue(ident, globalDecl);
            } else {
                Alloca localDecl = new Alloca(isInt);
                new Store((new ConstNumber(val)).withType(isInt), localDecl);

                symbolTableManager.setIRValue(ident, localDecl);
            }
            return;
        }

        if (irManager.inGlobalDecl()) {
            IRManager.getInstance().setAutoInsert(false);
            GlobalDecl globalDecl;
            ArrayInitializer arrayInitializer = (ArrayInitializer) constInitVal.toIR(ident, 0).withType(isInt);
            globalDecl = new GlobalDecl(arrayInitializer, isInt);
            symbolTableManager.arrayInit(ident, arrayInitializer);

            symbolTableManager.setIRValue(ident, globalDecl);
            IRManager.getInstance().setAutoInsert(true);
        } else {
            Alloca alloca = new Alloca(flattenLen, isInt);
            ArrayInitializer aInit = ((ArrayInitializer) (constInitVal.toIR(ident, 0)));
            HashMap<Integer, Value> initVals = aInit.getVals();
            if (initVals.isEmpty()) {
                ArrayList<Value> params = new ArrayList<>();
                params.add(new GetElementPtr(alloca, 0));
                params.add((new ConstNumber(0)).withType(isInt));
                params.add(new ConstNumber(4 * flattenLen));
                Function memset = (Function) SymbolTableManager.getInstance().getIRValue("@memset");
                new Call(memset, params);
            } else {
                for (int i = 0; i < aInit.getLen(); i++) {
                    Value addr = new GetElementPtr(alloca, i);
                    new Store((initVals.getOrDefault(i, new ConstNumber(0))).withType(isInt), addr);
                }
            }

            symbolTableManager.setIRValue(ident, alloca);
        }
    }
}
