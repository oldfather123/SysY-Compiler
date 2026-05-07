package midend.DCE;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.convop.Bitcast;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.AtomaticAddInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.objecttype.derived.TPointer;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.AliasAnalysis;
import midend.Analysis.LoopInfo;
import midend.FunctionInfo;
import midend.FunctionInfoCollector;

import java.util.*;

/**
 * StoreCSE - 利用别名分析的跨基本块冗余 Store 消除优化
 * 消除跨基本块的冗余 store 指令，即前一个 store 的写入值在逻辑上永远不会被使用，
 * 且在后续被一个新的 store 完全覆盖，从而可以安全删除前者。
 * 必要的准备工作：MemoryPass
 * 前置 Pass：CFG, LoopAnalysis, PureFunctionAnalysis
 * === 判定条件 ===
 * - `prev` 和 `current` 必须是对同一地址（must-alias）的写；
 * - `prev` 必须严格支配 `current`；
 * - `current` 必须支配对该地址的 所有可观察到的使用（包括 Load、AtomicAdd、可能读该地址的函数调用）；
 * - 在 `prev` 和 `current` 之间 不存在其他读取该地址的指令（包括 load、atomicAdd、调用具有副作用的函数）；
 * - 如果处于循环中，还需确保 不存在循环携带依赖，即该地址在循环体内不会被读取。
 */
public class StoreCSE extends MemoryPass {
    // 数组若作为函数参数，只会表现为传入第一个gep，但是！其他地址也会被用到。所以callInstr是其他地址隐形的使用者
    // baseAddr->CallInstr
    private static final Map<Value, List<CallInstr>> invisibleUser = new HashMap<>();

    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            if (function instanceof Function.LibFunc) continue;

            invisibleUser.clear();
            AliasAnalysis.curFunction = function;
            runOnFunc(function);
        }
    }

    public static void runOnFunc(Function function) {
        Map<BasicBlock, Map<Value, StoreInstr>> availIn = new HashMap<>();

        // 预计算每个基本块所属的最内层循环
        buildBlockToLoopMap(function);
        // 预处理invisible user
        filterCallInstrs(function);


        for (BasicBlock block : FunctionInfoCollector.getFuncInfo(function).getDomBfsOrder()) {
            Map<Value, StoreInstr> inMap = new HashMap<>();
            BasicBlock idom = block.getIDom();
            if (idom != null && availIn.containsKey(idom)) {
                inMap.putAll(availIn.get(idom));
            }

            // 处理循环头的特殊情况
            LoopInfo currentLoop = blockToInnermostLoop.get(block);
            if (currentLoop != null && currentLoop.getLoopHeader() == block) {
                inMap = filterStoreAtLoopHeader(currentLoop, inMap);
            }

            Map<Value, StoreInstr> curMap = new HashMap<>(inMap);
            List<Instruction> toRemove = new ArrayList<>();

            for (Instruction instr : new ArrayList<>(block.getInstrList())) {
                if (instr instanceof CallInstr callInstr) {
                    handleFunctionCall(callInstr, curMap);
                } else if (instr instanceof StoreInstr storeInstr) {
                    Value storeAddr = storeInstr.getPointer();
                    StoreInstr prev = curMap.get(storeAddr);

                    if (prev != null && canSafelyReplace(prev, storeInstr, blockToInnermostLoop)) {
                        toRemove.add(prev);
                        prev.replaceUseWith(instr);
                    }
                    curMap.put(storeAddr, storeInstr);

                }
            }

            for (Instruction del : toRemove) {
                del.forceRemoveFromList();
            }
            availIn.put(block, curMap);
        }
    }

    private static Value getBaseAddr(Value val) {
        Value baseAddr = val;
        while (baseAddr instanceof Bitcast || baseAddr instanceof GEPInstr) {
            if (baseAddr instanceof Bitcast bitcast) baseAddr = bitcast.getValue();
            if (baseAddr instanceof GEPInstr gep) baseAddr = gep.getPtrVal();
        }
        return baseAddr;
    }

    private static void filterCallInstrs(Function function) {
        for (BasicBlock block : function.getBasicBlockList()) {
            for (Instruction instr : block.getInstrList()) {
                if (instr instanceof CallInstr callInstr) {
                    Function callee = callInstr.getCallee();

                    FunctionInfo info = FunctionInfoCollector.getFuncInfo(callee);
                    if (info != null && info.isExternRead(callInstr)) {
                        for (Value rParam : callInstr.getRParams()) {
                            if (rParam.getType() instanceof TPointer) {
                                invisibleUser
                                        .computeIfAbsent(getBaseAddr(rParam), k -> new ArrayList<>())
                                        .add(callInstr);

                            }
                        }
                    }
                }
            }
        }
    }

    /**
     * 处理循环头：移除可能在循环中被读取的store
     *
     * @param loop
     * @param inMap
     * @return
     */

    private static Map<Value, StoreInstr> filterStoreAtLoopHeader(LoopInfo loop, Map<Value, StoreInstr> inMap) {
        Set<Value> readAddress = collectReadAddressInLoop(loop);
        Map<Value, StoreInstr> result = new HashMap<>(inMap);
        result.entrySet().removeIf(e -> readAddress.stream().anyMatch(m -> !AliasAnalysis.notAlias(e.getKey(), m)));
        return result;
    }

    private static Set<Value> collectReadAddressInLoop(LoopInfo loop) {
        Set<Value> result = new HashSet<>();
        HashSet<BasicBlock> blocks = new HashSet<>();
        loop.getAllBlocks(blocks);
        blocks.add(loop.getLoopHeader());

        for (BasicBlock bb : blocks) {
            for (Instruction instr : bb.getInstrList()) {
                if (instr instanceof AtomaticAddInstr atomicAdd) {
                    result.add(atomicAdd.getPtr());
                } else if (instr instanceof LoadInstr loadInstr) {
                    result.add(loadInstr.getPointer());
                } else if (instr instanceof CallInstr callInstr) {
                    var info = FunctionInfoCollector.getFuncInfo(callInstr.getCallee());
                    if (info != null) result.addAll(info.getMayRead(callInstr));
                }
            }
        }
        return result;
    }

    /**
     * current必须支配 【使用（最base的addr）的所有user（除了prev）】
     * 目标：无论从哪条执行路径观察，store1 的写入永远不会被“看见”或使用到。
     * 也就是说，store2 的写入一定会“覆盖”掉 store1 的写入。
     *
     * @param prev
     * @param current
     * @return
     */
    private static boolean checkDominance(StoreInstr prev, StoreInstr current) {
        // 如果在同一基本块，那么curr一定能覆盖prev
        if (prev.getParentBB() == current.getParentBB()) return true;

        Value storeAddr = prev.getPointer();

        for (Value addrUser : storeAddr.getUserList()) {
            if (addrUser == prev || addrUser == current) continue;
            if (!(addrUser instanceof Instruction addrUserInstr))
                throw new RuntimeException("user " + addrUser.value2string() + " is not Instruction");
            if (!current.dominates(addrUserInstr)) {
                return false;
            }
        }

        Value baseAddr = getBaseAddr(storeAddr);
        if (invisibleUser.containsKey(baseAddr)) {
            for (CallInstr user : invisibleUser.get(baseAddr)) {
                if (!current.dominates(user)) {
                    return false;
                }
            }
        }

        return true;
    }

    /**
     * 检查是否可以安全地用current替换prev
     *
     * @param prev
     * @param current
     * @param blockToLoop
     * @return
     */

    private static boolean canSafelyReplace(StoreInstr prev, StoreInstr current,
                                            Map<BasicBlock, LoopInfo> blockToLoop) {
        // 基本检查

        if (!AliasAnalysis.mustAlias(current.getPointer(), prev.getPointer()) ||
                !prev.dominates(current) ||
                !checkDominance(prev, current)) {
            return false;
        }

        BasicBlock prevBB = prev.getParentBB();
        BasicBlock currentBB = current.getParentBB();

        // 获取两个load所在的循环信息
        LoopInfo prevLoop = blockToLoop.get(prevBB);
        LoopInfo currentLoop = blockToLoop.get(currentBB);

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
            // 其他复杂情况保守处理
            return false;
        }

        // 情况5：prev在循环中，current不在循环中（这种情况在支配树遍历中应该不会出现）
        return false;
    }

    /**
     * 检查循环中是否存在对指定地址的循环携带依赖
     *
     * @param addr
     * @param loop
     * @return
     */

    private static boolean hasLoopCarriedDependency(Value addr, LoopInfo loop) {
        // 检查循环体内是否有对相同地址的load
        HashSet<BasicBlock> allBlocks = new HashSet<>();
        loop.getAllBlocks(allBlocks);
        allBlocks.add(loop.getLoopHeader());

        for (BasicBlock block : allBlocks) {
            for (Instruction instr : block.getInstrList()) {
                if (instr instanceof AtomaticAddInstr atomicAdd) {
                    if (!noAliasSensitiveWithOffset(addr, atomicAdd.getPtr())) {
                        return true; // 存在循环携带依赖
                    }
                } else if (instr instanceof LoadInstr loadInstr) {
                    if (!noAliasSensitiveWithOffset(addr, loadInstr.getPointer())) {
                        return true; // 存在循环携带依赖
                    }
                } else if (instr instanceof CallInstr callInstr) {
                    var funcInfo = FunctionInfoCollector.getFuncInfo(callInstr.getCallee());
                    if (funcInfo != null) {
                        for (Value mayRead : funcInfo.getMayRead(callInstr)) {
                            if (!AliasAnalysis.notAlias(addr, mayRead)) {
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
     * 检查两个store之间是否有干扰指令
     *
     * @param prev
     * @param current
     * @return
     */

    private static boolean hasInterferingInstructions(StoreInstr prev, StoreInstr current) {
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

    private static boolean hasInterferingInstructionsInSameBlock(StoreInstr prev, StoreInstr current, Value addr) {
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
            if (foundPrev && mayReadAddress(instr, addr)) {
                return true;
            }
        }
        return false;
    }

    private static boolean hasInterferingInstructionsAcrossBlocks(StoreInstr prev, StoreInstr current, Value addr) {
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
            if (foundPrev && mayReadAddress(instr, addr)) {
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
                if (mayReadAddress(instr, addr)) {
                    return true;
                }
            }
        }

        // 最后检查currentBB中current之前的指令
        for (Instruction instr : currentBB.getInstrList()) {
            if (instr == current) {
                break;
            }
            if (mayReadAddress(instr, addr)) {
                return true;
            }
        }

        return false;
    }

    private static boolean mayReadAddress(Instruction instr, Value addr) {
        if (instr instanceof AtomaticAddInstr atomicAdd) {
            return !noAliasSensitiveWithOffset(addr, atomicAdd.getPtr());
        } else if (instr instanceof LoadInstr loadInstr) {
            return !noAliasSensitiveWithOffset(addr, loadInstr.getPointer());
        } else if (instr instanceof CallInstr callInstr) {
            var funcInfo = FunctionInfoCollector.getFuncInfo(callInstr.getCallee());
            if (funcInfo != null) {
                return funcInfo.getMayRead(callInstr).stream().anyMatch(mayWrite ->
                        !AliasAnalysis.notAlias(addr, mayWrite));
            }
            Function callee = callInstr.getCallee();
            if (callee instanceof Function.LibFunc && callee.getName().startsWith("put") || callee.getName().startsWith("start")) {
                for (Value param : callInstr.getRParams()) {
                    if (param.getType() instanceof TPointer && !AliasAnalysis.notAlias(addr, param)) {
                        return true;
                    }
                }
            }
            return true; // 保守估计：未知函数可能读取任何地址
        }
        return false;
    }

    private static void handleFunctionCall(CallInstr callInstr, Map<Value, StoreInstr> curMap) {
        Function callee = callInstr.getCallee();

        var funcInfo = FunctionInfoCollector.getFuncInfo(callee);
        if (funcInfo != null) {
            // 特殊处理IO函数，读入参数并输出的libFunc默认是读了的
            if (callee instanceof Function.LibFunc && callee.getName().startsWith("put") || callee.getName().startsWith("start")) {
                for (Value param : callInstr.getRParams()) {
                    if (param.getType() instanceof TPointer) {
                        curMap.entrySet().removeIf(entry ->
                                !AliasAnalysis.notAlias(entry.getKey(), param));
                    }
                }
            }
            // 其他函数
            else {
                for (Value mayReadAddr : funcInfo.getMayRead(callInstr)) {
                    curMap.entrySet().removeIf(entry ->
                            !AliasAnalysis.notAlias(entry.getKey(), mayReadAddr));
                }
            }
        } else {
            // 未知函数，保守处理：清除所有store
            curMap.clear();
        }
    }
}
