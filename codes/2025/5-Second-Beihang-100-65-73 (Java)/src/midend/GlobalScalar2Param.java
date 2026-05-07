package midend;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.objecttype.derived.TPointer;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.symbol.GlbObjPtr;

import java.util.HashMap;
import java.util.HashSet;
import java.util.List;

/**
 * 如果一个函数调用期间没有修改global，只是读了global，且有递归调用，则可认为将global load放在函数外是提升的
 */
public class GlobalScalar2Param {
    private static final HashMap<GlbObjPtr, HashSet<LoadInstr>> accessMap = new HashMap<>();
    private static final int MAX_PARMS = 7;
    private static boolean overflow = false;
    private static boolean infoValid = false;

    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            overflow = false;
            tryToTransformFunc(function);
        }
    }

    public static void tryToTransformFunc(Function function) {
        if (function.getFParams().size() > MAX_PARMS) {
            return;
        }
        accessMap.clear();
        FunctionInfo functionInfo = FunctionInfoCollector.getFuncInfo(function);
        if (functionInfo.canGlobal2ParamTopLevel()) {
            accessAnalysis(function);
            infoValid = true;
            if (function.getFParams().size() + accessMap.keySet().size() > MAX_PARMS) {
                return;
            }
            transformFunc(function);
        }
    }

    public static void accessAnalysis(Function function) {
        for (BasicBlock bb : function.getBasicBlockList()) {
            for (Instruction ins : bb.getInstrList()) {
                if (ins instanceof LoadInstr load && load.getPointer() instanceof GlbObjPtr gp) {
                    accessMap.putIfAbsent(gp, new HashSet<>());
                    accessMap.get(gp).add(load);
                }
            }
        }
    }

    public static void transformFunc(Function function) {
        for (Function subFunction : function.getCalleeMap().keySet()) {
            if (subFunction == function) {
                continue;
            }
            FunctionInfo subFunctionInfo = FunctionInfoCollector.getFuncInfo(subFunction);
            if (subFunctionInfo.canGlobal2Param()) {
                transformFunc(subFunction);
            }
        }
        if (overflow) {
            return;
        }

        if (!infoValid) {
            accessMap.clear();
            accessAnalysis(function);
            infoValid = true;
        }
        if (accessMap.keySet().size() + function.getFParams().size() > MAX_PARMS) {
            overflow = true;
            return;
        }

        for (GlbObjPtr gp : accessMap.keySet()) {
            Function.FParam newFP = new Function.FParam(
                    ((TPointer)gp.getType()).getReferencedType(), function.getAndAddRegIdx(), true);
            function.appendFParam(newFP);

            for (LoadInstr load : accessMap.get(gp)) {
                load.replaceUseWith(newFP);
                load.removeFromList();
            }

            List<CallInstr> callInstrList = function.getCallInstrList();
            for (CallInstr callInstr : callInstrList) {
                if (callInstr.getCallee() == function) {
                    Value newParam;
                    if (callInstr.getParentBB().getParentFunc() != function) {
                        newParam = new LoadInstr(
                                callInstr.getParentBB().getParentFunc().getAndAddRegIdx(),
                                ((TPointer)gp.getType()).getReferencedType(), gp, callInstr.getParentBB());
                        newParam.setUse(gp);
                        newParam.insertBefore(callInstr);
                    } else {
                        newParam = newFP;
                    }
                    callInstr.getRParams().add(newParam);
                    callInstr.setUse(newParam);
                } else {
                    throw new RuntimeException("callInstr.getCallee() != function");
                }
            }
        }
        infoValid = false;
    }


}
