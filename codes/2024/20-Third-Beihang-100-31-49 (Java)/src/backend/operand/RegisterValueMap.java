package backend.operand;

import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.Instruction.Call;
import mid.IntermediatePresentation.Value;
import utils.Config;

import java.util.HashMap;
import java.util.HashSet;

public class RegisterValueMap {
    /*
        1. 寄存器使用情况记录
        2. 寄存器分配
     */

    private static final RegisterValueMap INSTANCE = new RegisterValueMap();
    private final HashMap<Function, HashMap<Value, BackIReg>> regOfVal = new HashMap<>();

    private Function curFunction;

    private RegisterValueMap() {
    }

    public static RegisterValueMap instance() {
        return INSTANCE;
    }

    public BackIReg getRegOf(Value v) {
        //这里从当前的map中取得
        return regOfVal.get(curFunction).getOrDefault(v, null);
    }

    public void setRegOf(Value v, BackIReg backIReg) {
        HashMap<Value, BackIReg> curFunctionRegMap = regOfVal.get(curFunction);
        if (!curFunctionRegMap.containsKey(v) || curFunctionRegMap.get(v) == null) {
            regOfVal.get(curFunction).put(v, backIReg);
        }
    }

    public void setCurFunction(Function function) {
        curFunction = function;
        if (!regOfVal.containsKey(function)) {
            regOfVal.put(function, new HashMap<>());
        }
    }

    public HashMap<Value, BackIReg> getAllRegs() {
        return regOfVal.get(curFunction);
    }

    public HashSet<BackIReg> getActiveRegs(Call call) {
//        if (Config.opt) {
            HashSet<BackIReg> activeRegs = new HashSet<>();
            for (Value v : RegisterAlloc.INSTANCE.activeValuesWhenCall(call)) {
                if (regOfVal.get(curFunction).containsKey(v)) {
                    activeRegs.add(regOfVal.get(curFunction).get(v));
                }
            }
            return activeRegs;
//        } else {
//            return new HashSet<>(regOfVal.get(curFunction).values());
//        }
    }

    public void clear() {
        regOfVal.get(curFunction).clear();
    }
}
