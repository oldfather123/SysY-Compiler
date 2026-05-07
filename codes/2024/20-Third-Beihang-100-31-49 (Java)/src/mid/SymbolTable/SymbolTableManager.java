package mid.SymbolTable;

import mid.IntermediatePresentation.Array.ArrayInitializer;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.GlobalDecl;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;


import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;

public class SymbolTableManager {
    private static final SymbolTableManager INSTANCE = new SymbolTableManager();
    private final LinkedList<SymbolTable> symbolTableStack = new LinkedList<>();
    private SymbolTable curTable;
    private int cycleLevel = 0;

    private FuncElem presFunc = null;
    private boolean returnTypeWhenCheck = false;


    public static SymbolTableManager getInstance() {
        return INSTANCE;
    }

    private SymbolTableManager() {
        curTable = new SymbolTable();
        symbolTableStack.push(curTable);
    }

    public void enterBlock() {
        curTable = new SymbolTable();
        symbolTableStack.push(curTable);
    }

    public void exitBlock() {
        symbolTableStack.pop();
        curTable = symbolTableStack.get(0);
    }

    public void enterCycle() {
        cycleLevel++;
    }

    public void exitCycle() {
        cycleLevel--;
        IRManager.getInstance().exitCycle();
    }

    public void funcDeclEnd() {
        presFunc = null;
        exitBlock();
        IRManager.getInstance().resetBlockCount();
    }

    public boolean notInCycle() {
        return cycleLevel == 0;
    }

    public int getCycleLevel() {
        return cycleLevel;
    }

    public boolean notDeclaredInCurLevel(String ident) {
        return !curTable.hasDeclared(ident);
    }

    public boolean notDeclared(String ident) {
        for (SymbolTable table : symbolTableStack) {
            if (table.hasDeclared(ident)) {
                return false;
            }
        }
        return true;
    }

    public void varDecl(String ident, boolean isConst, int dim, ArrayList<Integer> lens) {
        curTable.varDecl(ident, isConst, dim, lens);
    }

    public void funcDecl(ValueType retType, String ident) {
        curTable.funcDecl(retType, ident);
        presFunc = getFunction(ident);
        symbolTableStack.get(symbolTableStack.size() - 1).resetValForGlobalVar();
    }


    private FuncElem getFunction(String ident) {
        for (SymbolTable symbolTable : symbolTableStack) {
            if (symbolTable.getFunction(ident) != null) {
                return symbolTable.getFunction(ident);
            }
        }
        return null;
    }

    public ValueType getFunctionType(String ident) {
        FuncElem funcElem = getFunction(ident);
        if (funcElem == null) {
            return null;
        } else {
            return funcElem.getRetType();
        }
    }

    private VarElem getVar(String ident) {
        for (SymbolTable symbolTable : symbolTableStack) {
            if (symbolTable.getVar(ident) != null) {
                return symbolTable.getVar(ident);
            }
        }
        return null;
    }

    public void setVal(String ident, Number val) {
        VarElem varElem = getVar(ident);
        if (varElem != null) {
            if (varElem.getIrValue() instanceof GlobalDecl && IRManager.getInstance().inGlobalDecl()
                    && varElem.getVal() != null) {
                varElem.setTempVal(val);
            } else {
                varElem.setVal(val);
            }
        }
    }

    public Number getVal(String ident) {
        VarElem varElem = getVar(ident);
        if (varElem != null && (varElem.isConst() ||
                varElem.getIrValue() instanceof GlobalDecl && IRManager.getInstance().inGlobalDecl())) {
            return varElem.getVal();
        } else {
            return null;
        }
    }

    public void setArrayVal(String ident, Integer val, int index) {
        VarElem varElem = getVar(ident);
        if (varElem != null) {
            varElem.setArrayVal(val, index);
        }
    }

    public Number getArrayVal(String ident, int index) {
        VarElem varElem = getVar(ident);
        if (varElem != null && varElem.getIrValue() instanceof GlobalDecl
                && (IRManager.getInstance().inGlobalDecl() || varElem.isConst())) {
            return varElem.getArrayVal(index);
        } else {
            return null;
        }
    }

    public void arrayInit(String ident, ArrayInitializer arrayInitializer) {
        HashMap<Integer, Number> elems = new HashMap<>();
        for (int i : arrayInitializer.getVals().keySet()) {
            elems.put(i, ((ConstNumber) arrayInitializer.getVals().get(i)).getVal());
        }
        VarElem varElem = getVar(ident);
        if (varElem != null) {
            varElem.arrayInit(elems);
        }
    }

    public Value getIRValue(String ident) {
        VarElem varElem = getVar(ident);
        if (varElem != null) {
            return varElem.getIrValue();
        } else {
            FuncElem funcElem = getFunction(ident);
            if (funcElem != null) {
                return funcElem.getFunctionIR();
            } else {
                return null;
            }
        }
    }

    public void setIRValue(String ident, Value reg) {
        VarElem varElem = getVar(ident);
        if (varElem != null) {
            varElem.setIrValue(reg);
        } else {
            FuncElem funcElem = getFunction(ident);
            if (funcElem != null) {
                funcElem.setFunctionIR((Function) reg);
            }
        }
    }

    public int getDim(String ident) {
        VarElem varElem = getVar(ident);
        return (varElem == null) ? 0 : varElem.getDim();
    }

    public boolean varIsConst(String ident) {
        VarElem varElem = getVar(ident);
        if (varElem == null) {
            return false;
        } else {
            return varElem.isConst();
        }
    }


    public void setReturnCheckWhenRedcel(boolean isVoid) {
        returnTypeWhenCheck = isVoid;
    }

    public ArrayList<Integer> getDimLens(String ident) {
        VarElem varElem = getVar(ident);
        try {
            return new ArrayList<>(varElem.getLens());
        } catch (NullPointerException e) {
            return null;
        }
    }

}
