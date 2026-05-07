package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.exception.ValueBeingUsedException;
import cn.edu.bit.newnewcc.ir.type.VoidType;
import cn.edu.bit.newnewcc.ir.value.*;
import cn.edu.bit.newnewcc.ir.value.instruction.*;
import cn.edu.bit.newnewcc.pass.ir.util.CallMap;
import cn.edu.bit.newnewcc.pass.ir.util.GlobalAddressDetector;

import java.util.*;
import java.util.function.Consumer;

/**
 * 消除 ir 中的无用代码
 * <p>
 * 此Pass的功能是UnreachableCodeEliminationPass的超集
 */
/* 思路如下：
 *
 * 定义基本副作用指令为：
 * 1. 从函数入口块可达的存储地址为全局变量的store指令
 * 2. 从函数入口块可达的调用外部函数的指令
 *
 * 定义传递副作用指令为：
 * 1. 从函数入口块可达的call指令，且被调用的函数有副作用
 *
 * 定义函数有副作用，当且仅当：
 * 1. 其包含副作用指令
 *
 * 定义基本有效指令为：
 * 1. 从函数入口块可达的副作用指令
 * 2. 从函数入口块可达的存储地址为局部变量的store指令
 * 3. 从函数入口块可达的控制流指令
 *
 * 定义传递有效指令为：
 * 1. 指令的结果被作为有效指令的操作数
 *
 * 步骤如下：
 * 1. 求出所有副作用指令
 * 2. 求出所有有效指令
 * 3. 删除所有非有效指令
 *
 */
// 缩写：se = side effect
public class DeadCodeEliminationPass {

    GlobalAddressDetector addressTypeCache = new GlobalAddressDetector();

    /// 可达指令 & 副作用指令搜索

    private final Set<Instruction> reachableInstructions = new HashSet<>();
    private final Set<BasicBlock> reachableBlocks = new HashSet<>();
    private final Set<Instruction> seInstructions = new HashSet<>();
    private final Set<Function> seFunctions = new HashSet<>();
    Queue<Function> seFunctionQueue = new ArrayDeque<>();

    /// 返回值优化
    private final Map<Function, Set<ReturnInst>> returnInstMap = new HashMap<>();
    private final Set<Function> rvUsedFunctions = new HashSet<>();

    private boolean isBasicSeInstruction(Instruction instruction) {
        // 1. 从函数入口块可达的存储地址为全局变量的store指令
        if (instruction instanceof StoreInst storeInst) {
            if (addressTypeCache.isGlobalAddress(storeInst.getAddressOperand())) {
                return true;
            }
        }
        // 2. 从函数入口块可达的调用外部函数的指令
        if (instruction instanceof CallInst callInst) {
            if (callInst.getCallee() instanceof ExternalFunction) {
                return true;
            }
        }
        return false;
    }

    private void addSeInstruction(Instruction instruction) {
        seInstructions.add(instruction);
        var function = instruction.getBasicBlock().getFunction();
        if (!seFunctions.contains(function)) {
            seFunctions.add(function);
            seFunctionQueue.add(function);
        }
    }

    private void dfsForReachableInstructions(BasicBlock basicBlock) {
        if (reachableBlocks.contains(basicBlock)) return;
        reachableBlocks.add(basicBlock);
        for (Instruction instruction : basicBlock.getInstructions()) {
            if (isBasicSeInstruction(instruction)) {
                addSeInstruction(instruction);
            }
            if (instruction instanceof ReturnInst returnInst) {
                returnInstMap.get(basicBlock.getFunction()).add(returnInst);
            }
            reachableInstructions.add(instruction);
        }
        for (BasicBlock exitBlock : basicBlock.getExitBlocks()) {
            dfsForReachableInstructions(exitBlock);
        }
    }

    private void findSeInstructions(Module module) {
        for (Function function : module.getFunctions()) {
            returnInstMap.put(function, new HashSet<>());
            dfsForReachableInstructions(function.getEntryBasicBlock());
        }
        while (!seFunctionQueue.isEmpty()) {
            var function = seFunctionQueue.remove();
            for (Operand operand : function.getUsages()) {
                if (reachableInstructions.contains(operand.getInstruction())) {
                    // 为什么要检查这么多呢？谁知道会出现什么奇怪的情况，还是限制死了不留坑
                    if (operand.getInstruction() instanceof CallInst callInst && callInst.getCallee() == function) {
                        addSeInstruction(callInst);
                    }
                }
            }
        }
    }

    /// 有效指令搜索

    private final Set<Instruction> validInstructions = new HashSet<>();
    private final Queue<Instruction> validInstructionQueue = new ArrayDeque<>();

    private boolean isBasicValidInstruction(Instruction instruction) {
        // 1. 从函数入口块可达的副作用指令
        if (seInstructions.contains(instruction)) {
            return true;
        }
        // 2. 从函数入口块可达的存储地址为局部变量的store指令
        if (instruction instanceof StoreInst storeInst) {
            var address = storeInst.getAddressOperand();
            if (!addressTypeCache.isGlobalAddress(address)) {
                return true;
            }
        }
        // 3. 从函数入口块可达的控制流指令
        if (instruction instanceof TerminateInst) {
            if (instruction instanceof ReturnInst returnInst) {
                return returnInst.getBasicBlock().getFunction().getValueName().equals("main");
            } else {
                return true;
            }
        }
        return false;
    }

    private void addValidInstructionBySe(Instruction instruction) {
        if (!validInstructions.contains(instruction)) {
            validInstructions.add(instruction);
            validInstructionQueue.add(instruction);
        }
    }

    private void addValidInstructionByOperand(Instruction instruction) {
        addValidInstructionBySe(instruction);
        if (instruction instanceof CallInst callInst &&
                callInst.getCallee() instanceof Function callee &&
                !rvUsedFunctions.contains(callee)) {
            rvUsedFunctions.add(callee);
            for (ReturnInst returnInst : returnInstMap.get(callee)) {
                addValidInstructionByOperand(returnInst);
            }
        }
    }

    private void findValidInstructions() {
        for (BasicBlock reachableBlock : reachableBlocks) {
            for (Instruction instruction : reachableBlock.getInstructions()) {
                if (isBasicValidInstruction(instruction)) {
                    addValidInstructionBySe(instruction);
                }
            }
        }
        while (!validInstructionQueue.isEmpty()) {
            var validInstruction = validInstructionQueue.remove();
            for (Operand operand : validInstruction.getOperandList()) {
                // sValidInstruction = spread valid instruction
                if (operand.getValue() instanceof Instruction sValidInstruction && reachableInstructions.contains(sValidInstruction)) {
                    addValidInstructionByOperand(sValidInstruction);
                }
            }
        }
    }

    private boolean removeInvalidInstruction(Module module) {
        var invalidInstructions = new ArrayList<Instruction>();
        var invalidBasicBlocks = new ArrayList<BasicBlock>();
        boolean changed = false;
        // 获取以上两个数组
        for (Function function : module.getFunctions()) {
            for (BasicBlock basicBlock : function.getBasicBlocks()) {
                for (Instruction instruction : basicBlock.getInstructions()) {
                    if (!validInstructions.contains(instruction)) {
                        if (instruction instanceof TerminateInst) {
                            if (instruction instanceof ReturnInst returnInst && reachableInstructions.contains(returnInst)) {
                                // 无效但可达的返回指令，其只有返回的作用，返回值不重要
                                if (returnInst.getReturnValueType() != VoidType.getInstance() &&
                                        !(returnInst.getReturnValue() instanceof Instruction rvInst &&
                                                validInstructions.contains(rvInst))) {
                                    var zeroValue = returnInst.getReturnValueType().getZeroInitialization();
                                    if (!returnInst.getReturnValue().equals(zeroValue)) {
                                        returnInst.setReturnValue(zeroValue);
                                        changed = true;
                                    }
                                }
                            } else {
                                // 若控制流指令无效，说明这个基本块不可达
                                invalidInstructions.add(instruction);
                                invalidBasicBlocks.add(instruction.getBasicBlock());
                            }
                        } else {
                            invalidInstructions.add(instruction);
                        }
                    }
                }
            }
        }
        // 清理基本块的出口信息
        for (BasicBlock invalidBasicBlock : invalidBasicBlocks) {
            for (BasicBlock exitBlock : invalidBasicBlock.getExitBlocks()) {
                exitBlock.removeEntryFromPhi(invalidBasicBlock);
            }
            // 原有 TerminateInstruction 不在这里 waste ，因为其已经被划入 invalidInstruction ，稍后会被清理
            invalidBasicBlock.setTerminateInstruction(new UnreachableInst());
        }
        // 清理操作数
        for (Instruction invalidInstruction : invalidInstructions) {
            invalidInstruction.clearOperands();
        }
        // 检查块和值是否依然在被引用
        for (BasicBlock invalidBasicBlock : invalidBasicBlocks) {
            if (invalidBasicBlock.getUsages().size() > 0) {
                throw new ValueBeingUsedException();
            }
        }
        for (Instruction invalidInstruction : invalidInstructions) {
            if (invalidInstruction.getUsages().size() > 0) {
                throw new ValueBeingUsedException();
            }
        }
        // 移除无效值和无效块
        for (BasicBlock invalidBasicBlock : invalidBasicBlocks) {
            invalidBasicBlock.removeFromFunction();
        }
        for (Instruction invalidInstruction : invalidInstructions) {
            invalidInstruction.waste();
        }
        changed |= (!invalidInstructions.isEmpty() || !invalidBasicBlocks.isEmpty());
        return changed;
    }

    private boolean runOnModule_(Module module) {
        findSeInstructions(module);
        findValidInstructions();
        return removeInvalidInstruction(module);
    }

    private static void removeUnusedFunctions(Module module) {
        var calleeMap = CallMap.getCalleeMap(module);
        var unusedFunctions = new HashSet<>(module.getFunctions());
        // 使用 BFS 的方式从 main 开始探索所有可能被调用的函数
        Queue<Function> processQueue = new LinkedList<>();
        Consumer<Function> markReachable = function -> {
            if (unusedFunctions.contains(function)) {
                unusedFunctions.remove(function);
                processQueue.add(function);
            }
        };
        for (Function function : module.getFunctions()) {
            if (function.getValueName().equals("main")) {
                markReachable.accept(function);
                break;
            }
        }
        while (!processQueue.isEmpty()) {
            var caller = processQueue.remove();
            unusedFunctions.remove(caller);
            for (BaseFunction baseCallee : calleeMap.get(caller)) {
                if (baseCallee instanceof Function callee) {
                    markReachable.accept(callee);
                }
            }
        }
        for (Function unusedFunction : unusedFunctions) {
            destroyFunction(unusedFunction);
            module.removeFunction(unusedFunction);
        }
    }

    private static void destroyFunction(Function function) {
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            for (Instruction instruction : basicBlock.getInstructions()) {
                instruction.clearOperands();
            }
        }
    }

    public static boolean runOnModule(Module module) {
        removeUnusedFunctions(module);
        return new DeadCodeEliminationPass().runOnModule_(module);
    }

}
