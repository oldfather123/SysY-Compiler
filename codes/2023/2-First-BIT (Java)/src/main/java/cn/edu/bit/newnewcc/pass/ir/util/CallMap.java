package cn.edu.bit.newnewcc.pass.ir.util;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.value.*;
import cn.edu.bit.newnewcc.ir.value.instruction.CallInst;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class CallMap {

    public static Map<BaseFunction, Set<Function>> getCallerMap(Module module) {
        Map<BaseFunction, Set<Function>> callerMap = new HashMap<>();
        for (Function function : module.getFunctions()) {
            callerMap.put(function, new HashSet<>());
        }
        for (ExternalFunction externalFunction : module.getExternalFunctions()) {
            callerMap.put(externalFunction, new HashSet<>());
        }
        for (Function caller : module.getFunctions()) {
            for (BasicBlock basicBlock : caller.getBasicBlocks()) {
                for (Instruction instruction : basicBlock.getInstructions()) {
                    if (instruction instanceof CallInst callInst) {
                        var callee = callInst.getCallee();
                        callerMap.get(callee).add(caller);
                    }
                }
            }
        }
        return callerMap;
    }

    public static Map<Function, Set<BaseFunction>> getCalleeMap(Module module) {
        Map<Function, Set<BaseFunction>> calleeMap = new HashMap<>();
        for (Function function : module.getFunctions()) {
            calleeMap.put(function, new HashSet<>());
        }
        for (Function caller : module.getFunctions()) {
            for (BasicBlock basicBlock : caller.getBasicBlocks()) {
                for (Instruction instruction : basicBlock.getInstructions()) {
                    if (instruction instanceof CallInst callInst) {
                        var callee = callInst.getCallee();
                        calleeMap.get(caller).add(callee);
                    }
                }
            }
        }
        return calleeMap;
    }

}
