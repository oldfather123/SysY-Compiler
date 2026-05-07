package front.AST.Var;

import front.AST.Exp.ConstExp;
import front.AST.Node;
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

// VarDef ==> Ident { '[' ConstExp ']' } [ '=' InitVal ]

public class VarDef extends Node {

    public VarDef(SyntaxType sType, ArrayList<Node> children) {
        super(sType, children);
    }

    public void toIR(boolean isInt) {
        String ident = getTokenValue();
        int dim = 0;
        ArrayList<Integer> dimLens = new ArrayList<>();
        InitVal initVal = null;

        for (Node child : children) {
            if (child instanceof ConstExp constExp) {
                dim++;
                dimLens.add(constExp.evaluate().intValue());
            }
            if (child instanceof InitVal) {
                initVal = ((InitVal) child);
            }
        }

        symbolTableManager.varDecl(ident, false, dim, dimLens);
        int flattenLen = 1;
        for (Integer len : dimLens) {
            flattenLen *= len;
        }

        if (dim == 0) {
            if (initVal != null && !initVal.isEmpty()) {
                try {
                    //尝试在编译期求出初始值
                    Number val = initVal.evaluate();
                    if (isInt) {
                        val = val.intValue();
                    } else {
                        val = val.floatValue();
                    }
                    symbolTableManager.setVal(ident, val);

                    if (irManager.inGlobalDecl()) {
                        symbolTableManager.setIRValue(ident,
                                new GlobalDecl((new ConstNumber(val)).withType(isInt), isInt));
                    } else {
                        Alloca alloca = new Alloca(isInt);
                        new Store((new ConstNumber(val)).withType(isInt), alloca);
                        symbolTableManager.setIRValue(ident, alloca);
                    }
                } catch (NullPointerException e) {
                    if (IRManager.getInstance().inGlobalDecl()) {
                        symbolTableManager.setIRValue(ident,
                                new GlobalDecl((children.get(2).toIR()).withType(isInt), isInt));
                    } else {
                        Alloca localDecl = new Alloca(isInt);
                        new Store((initVal.toIR(ident, 0)).withType(isInt), localDecl);
                        symbolTableManager.setIRValue(ident, localDecl);
                    }
                }
            } else {
                if (IRManager.getInstance().inGlobalDecl()) {
                    symbolTableManager.setIRValue(ident, new GlobalDecl(
                            (new ConstNumber(0)).withType(isInt), isInt));
                } else {
                    symbolTableManager.setIRValue(ident, new Alloca(isInt));
                }
            }
            return;
        }

        if (irManager.inGlobalDecl()) {
            IRManager.getInstance().setAutoInsert(false);
            GlobalDecl globalDecl;
            if (initVal == null || initVal.isEmpty()) {
                globalDecl = new GlobalDecl((new ArrayInitializer(flattenLen)).withType(isInt), isInt);
            } else {
                ArrayInitializer arrayInitializer = (ArrayInitializer) initVal.toIR(ident, 0).withType(isInt);
                globalDecl = new GlobalDecl(arrayInitializer, isInt);
                symbolTableManager.arrayInit(ident, arrayInitializer);
            }
            symbolTableManager.setIRValue(ident, globalDecl);
            IRManager.getInstance().setAutoInsert(true);
        } else {
            Alloca alloca = new Alloca(flattenLen, isInt);
            if (initVal != null) {
                ArrayInitializer aInit = ((ArrayInitializer) (initVal.toIR(ident, 0)));
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
            }
            symbolTableManager.setIRValue(ident, alloca);
        }
    }
}