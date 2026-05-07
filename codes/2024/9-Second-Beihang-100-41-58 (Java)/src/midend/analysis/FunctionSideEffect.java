package midend.analysis;

import midend.*;
import midend.LLVMType.PointerType;
import midend.Module;
import midend.instr.CallInstr;
import midend.instr.GetElementPtrInstr;
import midend.instr.Instruction;
import midend.instr.StoreInstr;

import java.util.HashSet;

public class FunctionSideEffect {
    private static HashSet<Function> visited = new HashSet<>();
    public static void sideEffectAnalysis(Module module) {
        //TODO
        module.buildCallRelation();
        visited = new HashSet<>();
        for (Function function : module.getFunctions()) {
            if (function.getName().equals("@main")) {
                visited.add(function);
                function.setSideEffect();
                continue;
            }
            if (analysisFunc(function)) {
                setSideEffect(function);
            }
        }
    }

    //函数副作用分析：不会对内存和IO产生影响
    //1.不对全局变量以及数组指针进行改变
    //2.不调用库函数
    //3.若有副作用，则所有调用该函数的函数都有副作用
    //4.无副作用，则只会通过返回值影响
    public static boolean analysisFunc(Function function) {
        for (BasicBlock block : function.getBlockList()) {
//            if (block.equals(function.getBlockList().get(0))) {
//                continue;
//            }
            for (Instruction instruction : block.getInstrList()) {
                if (instruction instanceof CallInstr) {
                    Function callFunc = ((CallInstr) instruction).getFunction();
                    if (callFunc.isLib()) {
                        function.setSideEffect();
                        return true;
                    } else {
                        if (callFunc.isSideEffect()) {
                            function.setSideEffect();
                            return true;
                        }
                    }
                } else if (instruction instanceof StoreInstr) {
                    if (((StoreInstr) instruction).getPointer() instanceof GlobalVar ||
                            (((StoreInstr) instruction).getPointer() instanceof GetElementPtrInstr && ((GetElementPtrInstr) ((StoreInstr) instruction).getPointer()).getTarget() instanceof GlobalVar)) {
                        function.setSideEffect();
                        return true;
                    } else { //数组
                        if (((StoreInstr) instruction).getValue() instanceof Param && ((StoreInstr) instruction).getValue().getType() instanceof PointerType) {
                            function.setSideEffect();
                            return true;
                        }
                        if (((StoreInstr) instruction).getValue() instanceof GetElementPtrInstr && ((GetElementPtrInstr) ((StoreInstr) instruction).getValue()).getTarget() instanceof Param) {
                            function.setSideEffect();
                            return true;
                        }
                        if (((StoreInstr) instruction).getPointer() instanceof GetElementPtrInstr && ((GetElementPtrInstr) ((StoreInstr) instruction).getPointer()).getTarget() instanceof Param) {
                            function.setSideEffect();
                            return true;
                        }
                    }
                }
            }
        }
        //到这里基本就没有副作用了
        return false;
    }

    public static void setSideEffect(Function function) {
        if (visited.contains(function)) {
            return;
        }
        visited.add(function);
        for (Function function1 : function.getBeCalledList()) {
            function1.setSideEffect();
            setSideEffect(function1);
        }
    }
}
