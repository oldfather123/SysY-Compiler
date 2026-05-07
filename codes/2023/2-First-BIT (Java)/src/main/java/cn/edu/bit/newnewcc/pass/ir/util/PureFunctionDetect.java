package cn.edu.bit.newnewcc.pass.ir.util;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.exception.CompilationProcessCheckFailedException;
import cn.edu.bit.newnewcc.ir.type.PointerType;
import cn.edu.bit.newnewcc.ir.value.*;
import cn.edu.bit.newnewcc.ir.value.instruction.StoreInst;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class PureFunctionDetect {
    private static class Detector {

        private final Module module;
        private final Map<Function, Set<Function>> callers = new HashMap<>();
        private final Set<Function> externalCallers = new HashSet<>();
        private final Set<Function> nonPureFunctions = new HashSet<>();
        private final GlobalAddressDetector globalAddressDetector = new GlobalAddressDetector();

        public Detector(Module module) {
            this.module = module;
        }

        /**
         * 分析每个函数被哪些函数调用
         */
        private void analysisCallers() {
            var callMap = CallMap.getCallerMap(module);
            callMap.forEach((baseCallee, callers) -> {
                if (baseCallee instanceof Function callee) {
                    this.callers.put(callee, callers);
                } else if (baseCallee instanceof ExternalFunction) {
                    externalCallers.addAll(callers);
                } else {
                    throw new CompilationProcessCheckFailedException("Unknown class of function: " + baseCallee);
                }
            });
        }

        /**
         * 标记一个函数不是纯函数
         *
         * @param function 被标记的函数
         */
        private void addNonPureFunction(Function function) {
            if (nonPureFunctions.contains(function)) return;
            nonPureFunctions.add(function);
            for (Function caller : callers.get(function)) {
                addNonPureFunction(caller);
            }
        }

        /**
         * 检查某个函数是否为纯函数 <br>
         * 此方法<b>不考虑</b>调用非纯函数导致本函数变成非纯函数的情况 <br>
         *
         * @param function 被检查的函数
         * @return 若被检查的函数是纯函数，返回true；否则返回false。
         */
        // 导致非纯函数的原因包括：
        // 1. 调用非纯函数
        // 2. 引用全局变量
        // 3. 传入的参数包含指针
        // 4. 向除局部变量以外的位置store
        private boolean isPureFunction(Function function) {
            // 3. 传入的参数包含指针
            for (Function.FormalParameter formalParameter : function.getFormalParameters()) {
                if (formalParameter.getType() instanceof PointerType) return false;
            }
            for (BasicBlock basicBlock : function.getBasicBlocks()) {
                for (Instruction instruction : basicBlock.getInstructions()) {
                    // 2. 引用全局变量
                    for (Operand operand : instruction.getOperandList()) {
                        if (operand.getValue() instanceof GlobalVariable) {
                            return false;
                        }
                    }
                    // 4. 向除局部变量以外的位置store
                    if (instruction instanceof StoreInst storeInst) {
                        if (globalAddressDetector.isGlobalAddress(storeInst.getAddressOperand())) {
                            return false;
                        }
                    }
                }
            }
            return true;
        }

        public Set<Function> getPureFunctions() {
            analysisCallers();
            // 外部函数全部认为是非纯函数
            // 调用外部函数的都不是纯函数
            for (Function externalCaller : externalCallers) {
                addNonPureFunction(externalCaller);
            }
            // 检查每个函数是否为纯函数
            for (Function function : module.getFunctions()) {
                if (!isPureFunction(function)) {
                    addNonPureFunction(function);
                }
            }
            // 生成结果
            var result = new HashSet<Function>();
            for (Function function : module.getFunctions()) {
                if (!nonPureFunctions.contains(function)) {
                    result.add(function);
                }
            }
            return result;
        }

    }

    public static Set<Function> getPureFunctions(Module module) {
        return new Detector(module).getPureFunctions();
    }

}
