package midend.DCE;

import frontend.ir.Value;
import frontend.ir.constant.FloatConst;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.convop.Bitcast;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.AtomaticAddInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.objecttype.arithmetic.TInt;
import frontend.ir.objecttype.derived.TPointer;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.symbol.ArrayInitVal;
import frontend.ir.symbol.GlbObjPtr;
import frontend.ir.symbol.SymTab;
import frontend.ir.symbol.Symbol;
import midend.AliasAnalysis;
import midend.Analysis.LoopInfo;
import midend.FunctionInfo;
import midend.FunctionInfoCollector;

import java.util.*;

import static midend.DCE.MemoryPass.*;

/**
 * Store-to-Load Forwarding：
 * 对于被 dominator store 完全支配的 load 指令，且无其他干扰 store，则可用 store 的 value 替换 load。
 */
public class StoreToLoadForwardingV2 {
    // true：打開全局初值轉發
    private static boolean INIT_TRANSFER = true;

    private static SymTab globalSymTab;
    private static Function curFunction;
    //值为false：更激进的优化
    private static boolean debugging = false;

    private static final ArrayList<Instruction> delList = new ArrayList<>();


    // 局部数组: base -> (idx -> stored value)
    private static HashMap<Value, HashMap<Integer, DefPoint>> arrayStoreMap = new HashMap<>();
    // 局部数组已经记录的索引状态：base -> index status
    private static HashMap<Value, IndexStatus> arrayIndexMap = new HashMap<>();

    // 全局变量: gv -> stored value
    private static HashMap<GlbObjPtr, Value> globalStoreMap = new HashMap<>();
    // 全局变量的初值： gv -> initVal
    private static Map<Value, Value> global2InitValMap = new HashMap<>();
    // 被写的全局变量
    private static Set<GlbObjPtr> CURRENT_MUTABLE_GLOBALS = new HashSet<>();
    // 对非递归且无循环的函数，遇到写全局再添加进上面的集合
    private static boolean addInProcess = false;


    /**
     * 存储的定义点
     */
    public static class DefPoint {
        private Value storedValue; // 和指令里的值可能不一样，有可能是简化后的int值（仅对atomicAdd，但现在好像没有这个指令了?）
        private Instruction defInstr;

        public DefPoint(Value storedValue, Instruction defInstr) {
            this.storedValue = storedValue;
            this.defInstr = defInstr;
        }

        public Instruction getDefInstr() {
            return defInstr;
        }

        public Value getStoredValue() {
            return storedValue;
        }
    }

    private static Function getMainFunc(List<Function> functions) {
        for (Function function : functions) {
            if (function.getName().equals("main")) {
                return function;
            }
        }
        throw new RuntimeException("getMainFunc: there is no main function");
    }

    /**
     * 如果是数组initVal，就克隆，因为在简单SCC中可能只改变小索引
     */
    private static void genGlobal2InitValMap() {
        for (Symbol glbObj : globalSymTab.getObjectList()) {
            Value initVal = glbObj.getInitVal();
            if (initVal instanceof ArrayInitVal arrayInitVal) {
                initVal = arrayInitVal.cloneAndCalculateElementCnt();
            }
            global2InitValMap.put(glbObj.getPointer(), initVal);
        }
    }

    /**
     * 执行入口
     * @param functions 函数集
     * @param globalSymTab 全局符号表
     * @param INIT_TRANSFER 是否打开全局初值转发
     */
    public static void execute(List<Function> functions, SymTab globalSymTab, boolean INIT_TRANSFER) {
        StoreToLoadForwardingV2.globalSymTab = globalSymTab;
        StoreToLoadForwardingV2.INIT_TRANSFER = INIT_TRANSFER;
        genGlobal2InitValMap();

        // 1. 建调用图 & SCC & 拓扑序（从入口 main 出发）
        Map<Function, Set<Function>> cg = buildCallGraph(functions);
        Function entry = getMainFunc(functions);

        List<Set<Function>> sccs = scc(cg);
        List<Set<Function>> order = topoSccOrder(entry, cg, sccs);

        // 2. 一条线性序队列
        List<Function> linear = new ArrayList<>();
        for (Set<Function> comp : order) {
            // SCC 内函数顺序随意（DFS/原次序），但要在进入 SCC 前先“标记本 SCC 可能写的全局”
            linear.addAll(comp);
        }

        // 3. 维护：到目前为止“可能已被写”的全局集合
        Set<GlbObjPtr> mutableGlobals = new HashSet<>();
        // 预先给到 handleLoad 的判定函数
        CURRENT_MUTABLE_GLOBALS = mutableGlobals;

        // 4. 逐个“阶段（SCC）”处理
        for (Set<Function> comp : order) {
            // 4.1 先把该 SCC 的“可能写全局”并入（对递归安全）
            Set<GlbObjPtr> compWrites = new HashSet<>();
            for (Function f : comp) compWrites.addAll(mayWriteGlobals(f));
            if (!debugging) {
                if (comp.size() == 1 && comp.iterator().next() instanceof Function func &&
                        func.getAncientLoopInfo().isEmpty() &&
                        !FunctionInfoCollector.getFuncInfo(func).isRecur()) {
                    addInProcess = true;
                } else {
                    addInProcess = false;
                    mutableGlobals.addAll(compWrites);
                }
            }

            // 4.2 再依序跑 SCC 内函数的 StoreToLoadForwarding
            for (Function f : comp) {
                if (f instanceof Function.LibFunc) continue;
                AliasAnalysis.curFunction = f;
                StoreToLoadForwardingV2.curFunction = f;
                executeOnFunc(f); // 这里会在 handleLoad 中参考 CURRENT_MUTABLE_GLOBALS
                // 不在这里再加 mayWriteGlobals(f)，因为 4.1 已经做了“阶段前置杀死”，保证 SCC 内一致
            }
        }
    }


    private static void executeOnFunc(Function function) {
        delList.clear();
        Map<BasicBlock, HashMap<Value, HashMap<Integer, DefPoint>>> blk2ArrayStoreMap = new HashMap<>();
        Map<BasicBlock, Map<GlbObjPtr, Value>> blk2GlobalStoreMap = new HashMap<>();
        Map<BasicBlock, Map<Value, IndexStatus>> blk2ArrayIndexMap = new HashMap<>();

        buildBlockToLoopMap(function);

        for (BasicBlock block : FunctionInfoCollector.getFuncInfo(function).getDomBfsOrder()) {
            arrayStoreMap = new HashMap<>();
            globalStoreMap = new HashMap<>();
            arrayIndexMap = new HashMap<>();
            BasicBlock idom = block.getIDom();
            if (idom != null && blk2ArrayStoreMap.containsKey(idom)) {
                arrayStoreMap.putAll(cloneMap(blk2ArrayStoreMap.get(idom)));
                globalStoreMap.putAll(blk2GlobalStoreMap.get(idom));
                arrayIndexMap.putAll(blk2ArrayIndexMap.get(idom));
            }

            // 处理循环头的特殊情况
            LoopInfo currentLoop = blockToInnermostLoop.get(block);
            if (currentLoop != null && currentLoop.getLoopHeader() == block) {
                filterStoreAtLoopHeader(currentLoop); // 删掉arrayStoreMap和globalStoreMap中和[循环修改的地址]有冲突的值
            }

            for (Instruction instr : block.getInstrList()) {
                if (instr instanceof StoreInstr store) {
                    handleStore(store);
                } else if (instr instanceof LoadInstr load) {
                    handleLoad(load, blk2ArrayStoreMap, blk2GlobalStoreMap, blk2ArrayIndexMap);
                } else if (instr instanceof CallInstr call) {
                    handleCall(call);
                } else if (instr instanceof AtomaticAddInstr atomicAdd) {
                    handleAtomicAdd(atomicAdd);
                }
            }

            blk2ArrayStoreMap.put(block, arrayStoreMap);
            blk2GlobalStoreMap.put(block, globalStoreMap);
            blk2ArrayIndexMap.put(block, arrayIndexMap);
        }

        delList.forEach(Instruction::forceRemoveFromList);
    }

    /**
     * 处理循环头：从arrayStoreMap和globalStoreMap移除可能在循环中被读取的store
     */
    private static void filterStoreAtLoopHeader(LoopInfo loop) {
        // 收集当前循环层的所有块
        HashSet<BasicBlock> allBlocks = new HashSet<>();
        loop.getAllBlocks(allBlocks);
        allBlocks.add(loop.getLoopHeader());

        for (BasicBlock block : allBlocks) {
            for (Instruction instr : block.getInstrList()) {
                if (instr instanceof AtomaticAddInstr atomicAdd) {
                    filterAtomicAdd(atomicAdd);
                } else if (instr instanceof StoreInstr storeInstr) {
                    filterStore(storeInstr);
                } else if (instr instanceof CallInstr callInstr) {
                    handleCall(callInstr); // 清空
                }
            }
        }
    }

    /**
     * 移除该指令对应的store值
     */
    private static void filterAtomicAdd(AtomaticAddInstr atomicAdd) {
        Value addr = stripCasts(atomicAdd.getPtr());

        if (addr instanceof GEPInstr gep) {
            Value base = gep.getPtrVal();
            int key = encode(gep.getIndexList(), base);

            arrayStoreMap.putIfAbsent(base, new HashMap<>());
            if (arrayIndexMap.containsKey(base) && arrayIndexMap.get(base) == IndexStatus.INT && CURR_INDEX_STATUS == IndexStatus.INT) {
                // 当历史索引已知且现有索引已知时，可以只remove 单索引值
                arrayStoreMap.get(base).remove(key);
            } else {
                // 其他情况：不确定store位置，因此把整个base都移除
                arrayStoreMap.remove(base);
            }
        } else if (addr instanceof GlbObjPtr gv) {
            globalStoreMap.remove(gv); // 清空
        }
    }

    /**
     * 移除该指令对应的store值
     */
    private static void filterStore(StoreInstr store) {
        Value addr = stripCasts(store.getPointer());

        if (addr instanceof GEPInstr gep) {
            Value base = gep.getPtrVal();
            int key = encode(gep.getIndexList(), base);

            arrayStoreMap.putIfAbsent(base, new HashMap<>());
            if (arrayIndexMap.containsKey(base) && arrayIndexMap.get(base) == IndexStatus.INT && CURR_INDEX_STATUS == IndexStatus.INT) {
                // 当历史索引已知且现有索引已知时，可以只remove 单索引值
                arrayStoreMap.get(base).remove(key);
            } else {
                // 其他情况：不确定store位置，因此把整个base都移除
                arrayStoreMap.remove(base);
            }
        } else if (addr instanceof GlbObjPtr gv) {
            globalStoreMap.remove(gv); // 清空
        }
    }

    private static void handleAtomicAdd(AtomaticAddInstr atomicAdd) {
        Value addr = stripCasts(atomicAdd.getPtr());

        if (addr instanceof GEPInstr gep) {
            Value base = gep.getPtrVal();
            int key = encode(gep.getIndexList(), base);

            arrayStoreMap.putIfAbsent(base, new HashMap<>());
            if (arrayStoreMap.get(base).containsKey(key) &&
                    arrayStoreMap.get(base).get(key).getStoredValue() instanceof IntConst oldConstant &&
                    atomicAdd.getInc() instanceof IntConst plusConstant) {
                DefPoint defPoint = new DefPoint(new IntConst(oldConstant.getConstInt() + plusConstant.getConstInt()),
                        atomicAdd);
                arrayStoreMap.get(base).put(key, defPoint);
            } else {
                arrayStoreMap.get(base).clear(); // 保守清空，不放值
            }
        } else if (addr instanceof GlbObjPtr gv) {
            if (globalStoreMap.containsKey(gv) && globalStoreMap.get(gv) instanceof IntConst oldConstant &&
                    atomicAdd.getInc() instanceof IntConst plusConstant) {
                globalStoreMap.put(gv, new IntConst(oldConstant.getConstInt() + plusConstant.getConstInt()));
            } else {
                globalStoreMap.remove(gv); // 保守清空，不放值
            }
        }
    }

    private static void handleStore(StoreInstr store) {
        Value addr = stripCasts(store.getPointer());

        if (addr instanceof GEPInstr gep) {
            Value base = gep.getPtrVal();
            int key = encode(gep.getIndexList(), base);

            arrayStoreMap.putIfAbsent(base, new HashMap<>());

            IndexStatus prevIndexStatus = arrayIndexMap.getOrDefault(base, IndexStatus.EMPTY);
            if ((prevIndexStatus == IndexStatus.UNKNOWN && CURR_INDEX_STATUS == IndexStatus.INT) ||
                    (prevIndexStatus == IndexStatus.INT && CURR_INDEX_STATUS == IndexStatus.UNKNOWN)) {
                arrayStoreMap.get(base).clear(); // 保守清空
            }
            arrayStoreMap.get(base).put(key, new DefPoint(store.getValueToStore(), store));
            arrayIndexMap.put(base, CURR_INDEX_STATUS);

            if (addInProcess && getBase(addr) instanceof GlbObjPtr glbObjPtr) {
                if (CURR_INDEX_STATUS == IndexStatus.INT &&
                        global2InitValMap.get(glbObjPtr) instanceof ArrayInitVal arrayInitVal) {
                    if (arrayInitVal.replaceValue2NullWithIndexList(getIntIndexList(gep.getIndexList()))) {
                        CURRENT_MUTABLE_GLOBALS.add(glbObjPtr);
                    }

                } else {
                    CURRENT_MUTABLE_GLOBALS.add(glbObjPtr);
                }
            }

        } else if (addr instanceof GlbObjPtr gv) {
            globalStoreMap.put(gv, store.getValueToStore());

            if (addInProcess && getBase(addr) instanceof GlbObjPtr glbObjPtr) {
                CURRENT_MUTABLE_GLOBALS.add(glbObjPtr);
            }
        }
    }

    private static void handleLoad(LoadInstr load,
                                   Map<BasicBlock, HashMap<Value, HashMap<Integer, DefPoint>>> blk2ArrayStoreMap,
                                   Map<BasicBlock, Map<GlbObjPtr, Value>> blk2GlobalStoreMap,
                                   Map<BasicBlock, Map<Value, IndexStatus>> blk2ArrayIndexMap
    ) {


        Value addr = stripCasts(load.getPointer());

        if (addr instanceof GEPInstr gep) {
            Value base = gep.getPtrVal();
            List<Value> indexList = gep.getIndexList();
            int key = encode(indexList, base);

            // ===== 1) 先走你原有的前递（保持语义与收益） =====
            if (arrayStoreMap.containsKey(base) && arrayStoreMap.get(base).containsKey(key) &&
                    canSafelyReplace(arrayStoreMap.get(base).get(key).getDefInstr(), load)
            ) {
                Value val = arrayStoreMap.get(base).get(key).getStoredValue();
                load.replaceUseWith(val);
                delList.add(load);
                return;

            }

            // 尝试用“全局初值”前递
            if (base instanceof GlbObjPtr glbBase &&
                    CURR_INDEX_STATUS == IndexStatus.INT && canUseGlobalInitNow(glbBase) &&
                    global2InitValMap.get(glbBase) instanceof ArrayInitVal arrayInitVal &&
                    canInitSafelyReplace(load)) {

                List<Integer> intIndexList = getIntIndexList(indexList);

                Value val = arrayInitVal.getValueWithIndexList(intIndexList);

                if (val != null && !(val instanceof ArrayInitVal.ArrayZeroInitVal)) {
                    load.replaceUseWith(val);
                    delList.add(load);
                    return;
                }
            }

            // ===== 2) 局部 mem2reg：为该 GEP 槽位合成 phi =====
            if (tryPromoteLoadToPhiForGEP(load, gep, key, blk2ArrayStoreMap)) {
                delList.add(load);
            }

        } else if (addr instanceof GlbObjPtr gv) {
            // 原来的全局前递
            if (globalStoreMap.containsKey(gv)) {
                load.replaceUseWith(globalStoreMap.get(gv));
                delList.add(load);
                return;
            } else if (canUseGlobalInitNow(gv)) {
                load.replaceUseWith(global2InitValMap.get(gv));
                delList.add(load);
                return;
            }

            // 再尝试局部 mem2reg（全局标量槽位）
            if (tryPromoteLoadToPhiForGlobal(load, gv, blk2GlobalStoreMap)) {
                delList.add(load);
            }
        }
    }

    private static boolean tryPromoteLoadToPhiForGEP(
            LoadInstr load,
            GEPInstr gep,
            int key,
            Map<BasicBlock, HashMap<Value, HashMap<Integer, DefPoint>>> blk2ArrayStoreMap) {

        BasicBlock bb = load.getParentBB();
        Value base = gep.getPtrVal();

        // 仅在索引全为常量时才有把握
        // 若不是常量索引，直接放弃
        // 由于encode已经运算过一次了，这里直接用CURR_INDEX_STATUS判断
        if (CURR_INDEX_STATUS != IndexStatus.INT) return false;

        // 为每个前驱收集“该槽位的来值”
        Map<BasicBlock, Value> incoming = new LinkedHashMap<>();
        for (BasicBlock pred : bb.getPres()) {
            Value v = null;

            // 先看局部数组 map
            var amap = blk2ArrayStoreMap.get(pred);
            if (amap != null) {
                var byBase = amap.get(base);
                if (byBase != null) {
                    var dp = byBase.get(key);
                    if (dp != null) v = dp.getStoredValue();
                }
            }

            // 不做“全局初值兜底”：局部 mem2reg 只在每个前驱都能给出**确定**来值时进行

            // 如果这个 base 是全局数组，且局部没有来值，可以考虑全局的“标量写回”情况（很少）
            /*if (v == null && getBase(base) instanceof GlbObjPtr g) {
                var initVal = global2InitValMap.get(g);
                if (initVal != null && initVal instanceof ArrayInitVal arrayInitVal) {

                    List<Integer> intIndexList = getIntIndexList(gep.getIndexList());

                    Value val = arrayInitVal.getValueWithIndexList(intIndexList);
                    if (val != null) {
                        v = val; // 该全局的初始值
                    }
                }

            }*/

            if (v == null) {
                return false; // 某个前驱没有确定来值 → 放弃合成 phi
            }
            incoming.put(pred, v);
        }

        if (incoming.isEmpty()) {
            // 若无前驱值 → 放弃合成 phi
            return false;
        }

        // 若所有前驱来值都相同，直接替换，不必建 phi
        if (allSameValues(incoming.values())) {
            load.replaceUseWith(incoming.values().iterator().next());
            return true;
        }

        // 插入 phi（放在块头，与 IR 约定一致）
        PhiInstr phi = new PhiInstr(load.getType(), bb.getParentFunc().getAndAddRegIdx(), bb);
        bb.insertInsToHead(phi);

        for (Map.Entry<BasicBlock, Value> e : incoming.entrySet()) {
            phi.addOperand(e.getKey(), e.getValue());
        }

        load.replaceUseWith(phi);
        return true;
    }

    private static boolean tryPromoteLoadToPhiForGlobal(
            LoadInstr load,
            GlbObjPtr gv,
            Map<BasicBlock, Map<GlbObjPtr, Value>> blk2GlobalStoreMap) {

        BasicBlock bb = load.getParentBB();

        Map<BasicBlock, Value> incoming = new LinkedHashMap<>();
        for (BasicBlock pred : bb.getPres()) {
            Value v = null;

            var gmap = blk2GlobalStoreMap.get(pred);
            if (gmap != null) v = gmap.get(gv);

            // 注意：这里不使用“全局初值兜底”，局部 mem2reg 只在**每个前驱**有确定定义时进行
            if (v == null) return false;

            incoming.put(pred, v);
        }

        if (incoming.isEmpty()) return false;

        // 若所有前驱来值一致，直接替换
        if (allSameValues(incoming.values())) {
            load.replaceUseWith(incoming.values().iterator().next());
            return true;
        }

        // 合成 phi
        BasicBlock bbCur = load.getParentBB();
        PhiInstr phi = new PhiInstr(load.getType(), bbCur.getParentFunc().getAndAddRegIdx(), bbCur);
        bbCur.insertInsToHead(phi);

        for (Map.Entry<BasicBlock, Value> e : incoming.entrySet()) {
            phi.addOperand(e.getKey(), e.getValue());
        }

        load.replaceUseWith(phi);
        return true;
    }

    private static boolean allSameValues(Collection<Value> vs) {
        Value first = null;
        for (Value v : vs) {
            if (first == null) first = v;
            else if (first != v) return false;
        }
        return true;
    }


    private static boolean canUseGlobalInitNow(GlbObjPtr gv) {
        // 只有当它“尚未被任何已处理过的函数可能写”，才允许用初值
        if (debugging)
            return false;
        else
            return !CURRENT_MUTABLE_GLOBALS.contains(gv);
    }

    private static List<Integer> getIntIndexList(List<Value> indexList) {
        List<Integer> intIndexList = new ArrayList<>();
        for (int i = 1; i < indexList.size(); i++) {
            Value index = indexList.get(i);
            if (index instanceof IntConst vic) {
                intIndexList.add(vic.getConstInt().intValue());
            } else {
                throw new RuntimeException("handleLoad: non-INT index");
            }
        }
        return intIndexList;
    }

    private static void handleCall(CallInstr call) {
        FunctionInfo info = FunctionInfoCollector.getFuncInfo(call.getCallee());
        if (info.isParamWrite(call)) {
            for (Value arg : call.getRParams()) {
                if (arg.getType() instanceof TPointer) {
                    arrayStoreMap.remove(getBase(arg));
                    if (addInProcess && getBase(arg) instanceof GlbObjPtr glbObjPtr) {
                        CURRENT_MUTABLE_GLOBALS.add(glbObjPtr);
                    }
                }
            }
        }
        if (info.isGlobalWrite(call)) {
            for (Value value : info.getMayWrite(call)) {
                if (value instanceof GlbObjPtr gv) {
                    globalStoreMap.remove(gv);
                    if (addInProcess) {
                        CURRENT_MUTABLE_GLOBALS.add(gv);
                    }
                }
            }
        }
    }

    private static Value stripCasts(Value val) {
        while (val instanceof Bitcast bitCast) {
            val = bitCast.getValue();
        }
        return val;
    }

    private static Value getBase(Value val) {
        Value res = val;
        while (res instanceof GEPInstr || res instanceof Bitcast) {
            if (res instanceof GEPInstr gep) {
                res = gep.getPtrVal();
            }
            if (res instanceof Bitcast bitcast) {
                res = bitcast.getValue();
            }
        }
        return res;
    }

    private static HashMap<Value, HashMap<Integer, DefPoint>> cloneMap(HashMap<Value, HashMap<Integer, DefPoint>> map) {
        HashMap<Value, HashMap<Integer, DefPoint>> res = new HashMap<>();
        for (var e : map.entrySet()) {
            res.put(e.getKey(), new HashMap<>(e.getValue()));
        }
        return res;
    }

    private static boolean canInitSafelyReplace(LoadInstr current) {

        if (INIT_TRANSFER) {
            Instruction entryOfWholeFunction = curFunction.getFirstBlk().getFirstInstr();

            BasicBlock prevBB = entryOfWholeFunction.getParentBB();
            BasicBlock currentBB = current.getParentBB();

            // 获取两个load所在的循环信息
            LoopInfo prevLoop = blockToInnermostLoop.get(prevBB);
            LoopInfo currentLoop = blockToInnermostLoop.get(currentBB);

            // 情况1：两个load都不在任何循环中
            if (prevLoop == null && currentLoop == null) {
                return !hasInterferingInstructions(entryOfWholeFunction, current);
            }

            // 情况2：current在循环中，prev不在循环中
            if (prevLoop == null && currentLoop != null) {
                // 检查循环是否有循环携带依赖
                return !hasLoopCarriedDependency(current.getPointer(), currentLoop) &&
                        !hasInterferingInstructions(entryOfWholeFunction, current);
            }

            // 情况3：两个load都在同一个循环中
            if (prevLoop == currentLoop && prevLoop != null) {
                return !hasInterferingInstructions(entryOfWholeFunction, current);
            }

            // 情况4：两个load在不同的循环中
            if (prevLoop != currentLoop) {
                // 如果currentLoop是prevLoop的子循环
                if (isAncestorLoop(prevLoop, currentLoop)) {
                    return !hasLoopCarriedDependency(current.getPointer(), currentLoop) &&
                            !hasInterferingInstructions(entryOfWholeFunction, current);
                }
                // 其他复杂情况保守处理
                return false;
            }
        }

        // 情况5：prev在循环中，current不在循环中（这种情况在支配树遍历中应该不会出现）
        return false;
    }

    /**
     * 检查是否可以安全地用prev替换current
     */
    private static boolean canSafelyReplace(Instruction prev, LoadInstr current) {
        // 基本检查
        Value prevAddr;
        if (prev instanceof StoreInstr store) {
            prevAddr = store.getPointer();
        } else if (prev instanceof AtomaticAddInstr atomicAdd) {
            prevAddr = atomicAdd.getPtr();
        } else {
            return false;
        }

        if (!AliasAnalysis.mustAlias(current.getPointer(), prevAddr) || !prev.dominates(current)) {
            return false;
        }

        BasicBlock prevBB = prev.getParentBB();
        BasicBlock currentBB = current.getParentBB();

        // 获取两个instr所在的循环信息
        LoopInfo prevLoop = blockToInnermostLoop.get(prevBB);
        LoopInfo currentLoop = blockToInnermostLoop.get(currentBB);

        // 情况1：两个instr都不在任何循环中
        if (prevLoop == null && currentLoop == null) {
            return !hasInterferingInstructions(prev, current);
        }

        // 情况2：current在循环中，prev不在循环中
        if (prevLoop == null && currentLoop != null) {
            // 检查循环是否有循环携带依赖
            return !hasLoopCarriedDependency(current.getPointer(), currentLoop) &&
                    !hasInterferingInstructions(prev, current);
        }

        // 情况3：两个instr都在同一个循环中
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
    private static boolean hasInterferingInstructions(Instruction prev, LoadInstr current) {
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

    private static boolean hasInterferingInstructionsInSameBlock(Instruction prev, LoadInstr current, Value addr) {
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

    private static boolean hasInterferingInstructionsAcrossBlocks(Instruction prev, LoadInstr current, Value addr) {
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
            if (funcInfo == null) return true; // 保守估计：未知函数可能修改任何地址
            for (Value write : funcInfo.getMayWrite(callInstr)) {
                if (!AliasAnalysis.notAlias(addr, write)) {
                    return true;
                }
            }
            return false;
        } else {
            return false;
        }
    }

    private static Set<GlbObjPtr> mayWriteGlobals(Function f) {
        Set<GlbObjPtr> res = new HashSet<>();
        for (BasicBlock bb : f.getBasicBlockList()) {
            for (Instruction inst : bb.getInstrList()) {
                Value p = null;
                if (inst instanceof StoreInstr s) p = getBase(s.getPointer());
                else if (inst instanceof AtomaticAddInstr a) p = getBase(a.getPtr());
                if (p instanceof GlbObjPtr gv) res.add(gv);
                else if (inst instanceof CallInstr call) {
                    var info = FunctionInfoCollector.getFuncInfo(call.getCallee());
                    if (info != null && info.isGlobalWrite(call)) {
                        for (Value v : info.getMayWrite(call)) {
                            if (getBase(v) instanceof GlbObjPtr g) res.add(g);
                        }
                    } else {
                        // 未知副作用：看你策略。要极端保守可以认为“可能写任意全局”
                        // res.addAll(ALL_GLOBALS);
                    }
                }
            }
        }
        return res;
    }

    private static Map<Function, Set<Function>> buildCallGraph(Collection<Function> functions) {
        Map<Function, Set<Function>> g = new HashMap<>();
        for (Function f : functions) {
            if (f instanceof Function.LibFunc) continue;
            g.putIfAbsent(f, new HashSet<>());
            for (BasicBlock bb : f.getBasicBlockList()) {
                for (Instruction inst : bb.getInstrList()) {
                    if (inst instanceof CallInstr call) {
                        Function cal = call.getCallee();
                        if (cal instanceof Function.LibFunc) continue;
                        g.get(f).add(cal);
                    }
                }
            }
        }
        return g;
    }

    private static List<Set<Function>> scc(Map<Function, Set<Function>> g) {
        // Tarjan 简版
        Map<Function, Integer> idx = new HashMap<>();
        Map<Function, Integer> low = new HashMap<>();
        Deque<Function> st = new ArrayDeque<>();
        Set<Function> onst = new HashSet<>();
        List<Set<Function>> comps = new ArrayList<>();
        int[] time = {0};
        for (Function v : g.keySet()) {
            if (!idx.containsKey(v)) dfs(v, g, idx, low, st, onst, time, comps);
        }
        return comps;
    }

    private static void dfs(Function v, Map<Function, Set<Function>> g,
                            Map<Function, Integer> idx, Map<Function, Integer> low,
                            Deque<Function> st, Set<Function> onst, int[] time,
                            List<Set<Function>> comps) {
        idx.put(v, time[0]);
        low.put(v, time[0]);
        time[0]++;
        st.push(v);
        onst.add(v);
        for (Function w : g.getOrDefault(v, Collections.emptySet())) {
            if (!idx.containsKey(w)) {
                dfs(w, g, idx, low, st, onst, time, comps);
                low.put(v, Math.min(low.get(v), low.get(w)));
            } else if (onst.contains(w)) {
                low.put(v, Math.min(low.get(v), idx.get(w)));
            }
        }
        if (Objects.equals(low.get(v), idx.get(v))) {
            Set<Function> comp = new LinkedHashSet<>();
            while (true) {
                Function x = st.pop();
                onst.remove(x);
                comp.add(x);
                if (x == v) break;
            }
            comps.add(comp);
        }
    }

    /**
     * 返回按“从入口出发”的 SCC 拓扑序，每个元素是一个 SCC（函数集合）
     */
    private static List<Set<Function>> topoSccOrder(Function entry,
                                                    Map<Function, Set<Function>> g,
                                                    List<Set<Function>> comps) {
        // SCC id
        Map<Function, Integer> id = new HashMap<>();
        for (int i = 0; i < comps.size(); i++) {
            for (Function f : comps.get(i)) id.put(f, i);
        }
        // 建 SCC DAG
        int n = comps.size();
        List<Set<Integer>> dag = new ArrayList<>();
        for (int i = 0; i < n; i++) dag.add(new HashSet<>());
        for (var e : g.entrySet()) {
            int u = id.get(e.getKey());
            for (Function v : e.getValue()) {
                int w = id.get(v);
                if (u != w) dag.get(u).add(w);
            }
        }
        // 从 entry 所在 SCC 开始的可达子图做拓扑排序（Kahn）
        int entryId = id.getOrDefault(entry, -1);
        boolean[] reachable = new boolean[n];
        Deque<Integer> q = new ArrayDeque<>();
        if (entryId >= 0) {
            q.add(entryId);
            reachable[entryId] = true;
        }
        while (!q.isEmpty()) {
            int u = q.poll();
            for (int v : dag.get(u))
                if (!reachable[v]) {
                    reachable[v] = true;
                    q.add(v);
                }
        }
        int[] indeg = new int[n];
        for (int u = 0; u < n; u++)
            if (reachable[u]) {
                for (int v : dag.get(u)) if (reachable[v]) indeg[v]++;
            }
        Deque<Integer> z = new ArrayDeque<>();
        for (int i = 0; i < n; i++) if (reachable[i] && indeg[i] == 0) z.add(i);

        List<Set<Function>> order = new ArrayList<>();
        while (!z.isEmpty()) {
            int u = z.poll();
            order.add(comps.get(u));
            for (int v : dag.get(u))
                if (reachable[v]) {
                    if (--indeg[v] == 0) z.add(v);
                }
        }
        return order;
    }

}
