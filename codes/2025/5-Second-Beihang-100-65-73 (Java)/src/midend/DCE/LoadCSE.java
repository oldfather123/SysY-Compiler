package midend.DCE;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.AtomaticAddInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.AliasAnalysis;
import midend.Analysis.LoopInfo;
import midend.FunctionInfoCollector;

import java.util.*;

/**
 * LoadCSE - 利用别名分析的Load CSE 优化
 * 改进版本：结合现有的循环分析，正确处理循环中的load-store依赖
 * 前置：CFG, LoopAnalysis, PureFunctionAnalysis
 * 由于准备工作的存在，该pass必须是内存pass的首位
 */
public class LoadCSE extends MemoryPass {

    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            if (function instanceof Function.LibFunc) continue;

            // 确保循环分析已完成
            AliasAnalysis.curFunction = function;
            // 确保ReachabilityCache正确
            FunctionInfoCollector.getFuncInfo(function).refreshReachabilityCache();

            runOnFunc(function);
        }
    }

    public static void runOnFunc(Function function) {
        Map<BasicBlock, Map<Value, LoadInstr>> availIn = new HashMap<>();

        // 预计算每个基本块所属的最内层循环
        buildBlockToLoopMap(function);

        for (BasicBlock block : FunctionInfoCollector.getFuncInfo(function).getDomBfsOrder()) {
            Map<Value, LoadInstr> inMap = new HashMap<>();
            BasicBlock idom = block.getIDom();
            if (idom != null && availIn.containsKey(idom)) {
                inMap.putAll(availIn.get(idom));
            }

            // 处理循环头的特殊情况
            LoopInfo currentLoop = blockToInnermostLoop.get(block);
            if (currentLoop != null && currentLoop.getLoopHeader() == block) {
                inMap = handleLoopHeader(currentLoop, inMap);
            }

            Map<Value, LoadInstr> curMap = new HashMap<>(inMap);
            List<Instruction> toRemove = new ArrayList<>();

            for (Instruction instruction : new ArrayList<>(block.getInstrList())) {
                if (instruction instanceof CallInstr callInstr) {
                    handleFunctionCall(callInstr, curMap);
                } else if (instruction instanceof AtomaticAddInstr atomicAdd) {
                    Value atomicAddr = atomicAdd.getPtr();
                    curMap.entrySet().removeIf(entry ->
                            !noAliasSensitiveWithOffset(entry.getKey(), atomicAddr));
                    // todo alias check
                } else if (instruction instanceof StoreInstr storeInstr) {
                    Value storeAddr = storeInstr.getPointer();
                    curMap.entrySet().removeIf(entry ->
                            !noAliasSensitiveWithOffset(entry.getKey(), storeAddr));
                } else if (instruction instanceof LoadInstr loadInstr) {
                    Value loadAddr = loadInstr.getPointer();
                    LoadInstr prev = curMap.get(loadAddr);

                    if (prev != null && canSafelyReplace(prev, loadInstr)) {
                        instruction.replaceUseWith(prev);
                        toRemove.add(instruction);
                    } else {
                        curMap.put(loadAddr, loadInstr);
                    }
                }
            }

            for (Instruction del : toRemove) {
                del.forceRemoveFromList();
            }
            availIn.put(block, curMap);
        }
    }

    /**
     * 处理循环头：移除可能在循环中被修改的load
     */
    private static Map<Value, LoadInstr> handleLoopHeader(LoopInfo loop, Map<Value, LoadInstr> inMap) {
        Map<Value, LoadInstr> result = new HashMap<>(inMap);

        // 收集当前循环层的所有块
        HashSet<BasicBlock> allBlocks = new HashSet<>();
        loop.getAllBlocks(allBlocks);
        allBlocks.add(loop.getLoopHeader());

        for (BasicBlock block : allBlocks) {
            for (Instruction instr : block.getInstrList()) {
                if (instr instanceof AtomaticAddInstr atomicAdd) {
                    result.entrySet().removeIf(entry ->
                            !noAliasSensitiveWithOffset(entry.getKey(), atomicAdd.getPtr()));
                } else if (instr instanceof StoreInstr storeInstr) {
                    result.entrySet().removeIf(entry ->
                            !noAliasSensitiveWithOffset(entry.getKey(), storeInstr.getPointer()));
                } else if (instr instanceof CallInstr callInstr) {
                    var funcInfo = FunctionInfoCollector.getFuncInfo(callInstr.getCallee());
                    Set<Value> modifiedAddresses = new HashSet<>();
                    if (funcInfo != null) {
                        modifiedAddresses.addAll(funcInfo.getMayWrite(callInstr));
                        // 移除可能在循环中被修改的load
                        result.entrySet().removeIf(entry -> {
                            Value loadAddr = entry.getKey();
                            return modifiedAddresses.stream().anyMatch(modAddr ->
                                    !AliasAnalysis.notAlias(loadAddr, modAddr));
                        });
                    }
                }
            }
        }

        return result;
    }

    /**
     * 收集循环中所有可能被修改的地址
     */
    private static Set<Value> collectModifiedAddressesInLoop(LoopInfo loop) {
        Set<Value> modifiedAddresses = new HashSet<>();

        // 收集当前循环层的所有块
        HashSet<BasicBlock> allBlocks = new HashSet<>();
        loop.getAllBlocks(allBlocks);
        allBlocks.add(loop.getLoopHeader());

        for (BasicBlock block : allBlocks) {
            for (Instruction instr : block.getInstrList()) {
                if (instr instanceof AtomaticAddInstr atomicAdd) {
                    modifiedAddresses.add(atomicAdd.getPtr());
                } else if (instr instanceof StoreInstr storeInstr) {
                    modifiedAddresses.add(storeInstr.getPointer());
                } else if (instr instanceof CallInstr callInstr) {
                    var funcInfo = FunctionInfoCollector.getFuncInfo(callInstr.getCallee());
                    if (funcInfo != null) {
                        modifiedAddresses.addAll(funcInfo.getMayWrite(callInstr));
                    }
                }
            }
        }

        return modifiedAddresses;
    }

    /**
     * 检查是否可以安全地用prev替换current
     */
    private static boolean canSafelyReplace(LoadInstr prev, LoadInstr current) {
        // 基本检查
        if (!AliasAnalysis.mustAlias(current.getPointer(), prev.getPointer()) ||
                !prev.dominates(current) ||
                !current.isDominatedAllUsesBy(prev)) {
            return false;
        }

        BasicBlock prevBB = prev.getParentBB();
        BasicBlock currentBB = current.getParentBB();

        // 获取两个load所在的循环信息
        LoopInfo prevLoop = blockToInnermostLoop.get(prevBB);
        LoopInfo currentLoop = blockToInnermostLoop.get(currentBB);

        // 情况1：两个load都不在任何循环中
        if (prevLoop == null && currentLoop == null) {
            return !hasInterferingInstructions(prev, current);
        }

        // 情况2：current在循环中，prev不在循环中
        if (prevLoop == null && currentLoop != null) {
            // 检查循环是否有循环携带依赖
            return !hasLoopCarriedDependency(current.getPointer(), currentLoop) &&
                    !hasInterferingInstructions(prev, current);
        }

        // 情况3：两个load都在同一个循环中
        if (prevLoop == currentLoop && prevLoop != null) {
            return !hasInterferingInstructions(prev, current);
        }

        // 情况4：两个load在不同的循环中
        if (prevLoop != currentLoop) {
            // 如果currentLoop是prevLoop的子循环
            if (isAncestorLoop(prevLoop, currentLoop)) {
                return !hasLoopCarriedDependency(current.getPointer(), currentLoop) &&
                        !hasInterferingInstructions(prev, current);
            }

            // 如果currentLoop和prevLoop在同一个parentLoop下
            if (currentLoop != null && prevLoop != null &&
                    currentLoop.getParentLoop() == prevLoop.getParentLoop()) {
                return !hasLoopCarriedDependency(current.getPointer(), currentLoop) &&
                        !hasLoopCarriedDependency(current.getPointer(), prevLoop) &&
                        !hasInterferingInstructions(prev, current);
            }

            // 其他复杂情况
            return false;

        }

        // 情况5：prev在循环中，current不在循环中，大概率false，不算了
        return false;
    }


    /**
     * 检查循环中是否存在对指定地址的循环携带依赖
     */
    private static boolean hasLoopCarriedDependency(Value addr, LoopInfo loop) {
        // 检查循环体内是否有对相同地址的store
        HashSet<BasicBlock> allBlocks = new HashSet<>();
        loop.getAllBlocks(allBlocks);
        allBlocks.add(loop.getLoopHeader());

        for (BasicBlock block : allBlocks) {
            for (Instruction instr : block.getInstrList()) {
                if (instr instanceof AtomaticAddInstr atomicAdd) {
                    if (!noAliasSensitiveWithOffset(addr, atomicAdd.getPtr())) {
                        return true; // 存在循环携带依赖
                    }
                } else if (instr instanceof StoreInstr storeInstr) {
                    if (!noAliasSensitiveWithOffset(addr, storeInstr.getPointer())) {
                        return true; // 存在循环携带依赖
                    }
                } else if (instr instanceof CallInstr callInstr) {
                    var funcInfo = FunctionInfoCollector.getFuncInfo(callInstr.getCallee());
                    if (funcInfo != null) {
                        for (Value mayWrite : funcInfo.getMayWrite(callInstr)) {
                            if (!AliasAnalysis.notAlias(addr, mayWrite)) {
                                return true;
                            }
                        }
                    } else {
                        // 未知函数，保守假设可能修改该地址
                        return true;
                    }
                }
            }
        }

        return false;
    }

    /**
     * 检查两个load之间是否有干扰指令
     */
    private static boolean hasInterferingInstructions(LoadInstr prev, LoadInstr current) {
        BasicBlock prevBB = prev.getParentBB();
        BasicBlock currentBB = current.getParentBB();
        Value addr = current.getPointer();

        if (prevBB == currentBB) {
            // 同一基本块内检查
            return hasInterferingInstructionsInSameBlock(prev, current, addr);
        } else {
            // 跨基本块检查 - 使用支配关系进行路径分析
            return hasInterferingInstructionsAcrossBlocks(prev, current, addr);
        }
    }

    private static boolean hasInterferingInstructionsInSameBlock(LoadInstr prev, LoadInstr current, Value addr) {
        boolean foundPrev = false;
        BasicBlock block = prev.getParentBB();

        for (Instruction instr : block.getInstrList()) {
            if (instr == prev) {
                foundPrev = true;
                continue;
            }
            if (foundPrev && instr == current) {
                break;
            }
            if (foundPrev && mayModifyAddress(instr, addr)) {
                return true;
            }
        }
        return false;
    }

    private static boolean hasInterferingInstructionsAcrossBlocks(LoadInstr prev, LoadInstr current, Value addr) {
        // 简化实现：检查从prev所在块到current所在块路径上的所有块
        // 由于current被prev支配，我们需要检查从prev的下一条指令开始到current之间的所有指令

        BasicBlock prevBB = prev.getParentBB();
        BasicBlock currentBB = current.getParentBB();

        // 首先检查prevBB中prev之后的指令
        boolean foundPrev = false;
        for (Instruction instr : prevBB.getInstrList()) {
            if (instr == prev) {
                foundPrev = true;
                continue;
            }
            if (foundPrev && mayModifyAddress(instr, addr)) {
                return true;
            }
        }

        // 然后检查从prevBB到currentBB路径上的中间块
        // 实现：检查从prevBB到currentBB的所有路径的中间块，并集∪
        Set<BasicBlock> intermediateBlocks = FunctionInfoCollector.getFuncInfo(prevBB.getParentFunc())
                .getReachabilityCache().findIntermediateBlocksPro(prevBB, currentBB);
        if (intermediateBlocks == null) return true;
        for (BasicBlock block : intermediateBlocks) {
            for (Instruction instr : block.getInstrList()) {
                if (mayModifyAddress(instr, addr)) {
                    return true;
                }
            }
        }

        // 最后检查currentBB中current之前的指令
        for (Instruction instr : currentBB.getInstrList()) {
            if (instr == current) {
                break;
            }
            if (mayModifyAddress(instr, addr)) {
                return true;
            }
        }

        return false;
    }

    private static boolean mayModifyAddress(Instruction instr, Value addr) {
        if (instr instanceof AtomaticAddInstr atomicAdd) {
            return !noAliasSensitiveWithOffset(addr, atomicAdd.getPtr());
        } else if (instr instanceof StoreInstr storeInstr) {
            return !noAliasSensitiveWithOffset(addr, storeInstr.getPointer());
        } else if (instr instanceof CallInstr callInstr) {
            var funcInfo = FunctionInfoCollector.getFuncInfo(callInstr.getCallee());
            if (funcInfo != null) {
                return funcInfo.getMayWrite(callInstr).stream().anyMatch(mayWrite ->
                        !AliasAnalysis.notAlias(addr, mayWrite));
            }
            return true; // 保守估计：未知函数可能修改任何地址
        }
        return false;
    }

    private static void handleFunctionCall(CallInstr callInstr, Map<Value, LoadInstr> curMap) {
        var funcInfo = FunctionInfoCollector.getFuncInfo(callInstr.getCallee());
        if (funcInfo != null) {
            for (Value mayModifyAddr : funcInfo.getMayWrite(callInstr)) {
                curMap.entrySet().removeIf(entry ->
                        !AliasAnalysis.notAlias(entry.getKey(), mayModifyAddr));
            }
        } else {
            // 未知函数，保守处理：清除所有load
            curMap.clear();
        }
    }
}