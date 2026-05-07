package midend.DCE;

import frontend.ir.Value;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.AliasAnalysis;
import midend.Analysis.LoopInfo;

import java.util.*;

public class MemoryPass {

    public static void execute(List<Function> functions) {
    }

    // 所有函数的所有基本块 -> 最内层循环
    public static Map<BasicBlock, LoopInfo> blockToInnermostLoop = new HashMap<>();

    public static void buildBlockToLoopMap(Function function) {
        Map<BasicBlock, LoopInfo> blockToLoop = new HashMap<>();
        for (LoopInfo topLoop : function.getAncientLoopInfo()) {
            buildLoopRecursive(topLoop, blockToLoop);
        }
        blockToInnermostLoop.putAll(blockToLoop);
    }

    private static void buildLoopRecursive(LoopInfo loop, Map<BasicBlock, LoopInfo> map) {
        // 首先处理子循环（更内层）
        for (LoopInfo child : loop.getChildLoop()) {
            buildLoopRecursive(child, map);
        }
        // 然后处理当前循环的块（只有那些还没被更内层循环标记的）
        HashSet<BasicBlock> all = new HashSet<>();
        loop.getAllBlocks(all);
        all.add(loop.getLoopHeader()); // 确保包含循环头
        for (BasicBlock bb : all) {
            if (!map.containsKey(bb)) {
                map.put(bb, loop);
            }
        }
    }

    /**
     * 检查loop1是否是loop2的祖先循环
     */
    public static boolean isAncestorLoop(LoopInfo loop1, LoopInfo loop2) {
        if (loop1 == null || loop2 == null) {
            return false;
        }

        LoopInfo parent = loop2.getParentLoop();
        while (parent != null) {
            if (parent == loop1) {
                return true;
            }
            parent = parent.getParentLoop();
        }
        return false;
    }

    public enum IndexStatus {
        // indexList是未知的?全ConstInt?
        UNKNOWN, INT, EMPTY
    }

    public static IndexStatus CURR_INDEX_STATUS; // 当前indexStatus（还未记录）

    /**
     * 编码indexList 的唯一 key，会改变arrayIndexMap
     */
    public static int encode(List<Value> indexList, Value base) {
        // check 更新前arrayStoreMap状态 todo
//        if (!arrayStoreMap.containsKey(base) || (arrayStoreMap.containsKey(base) && arrayStoreMap.get(base).isEmpty())) {
//            arrayIndexMap.put(base, StoreToLoadForwarding.IndexStatus.EMPTY);
//        }
        CURR_INDEX_STATUS = IndexStatus.INT;

        List<Integer> constIntValues = new ArrayList<>();
        List<Integer> valueIds = new ArrayList<>();

        // 如果indexList全是constInt值，那么hash中装的都是constInt的value，即返回constIntValues
        // 如果indexList中含未知值，那么hash中装的是Value本身的id（专为这部分准备的hash），即返回valueIds
        for (Value index : indexList) {
            valueIds.add(index.getId());
            if (index instanceof IntConst idxInt) constIntValues.add(idxInt.getConstInt().intValue());
            else {
                CURR_INDEX_STATUS = IndexStatus.UNKNOWN;
            }
        }
        if (CURR_INDEX_STATUS == IndexStatus.UNKNOWN) {
            return valueIds.hashCode();
        } else {
            return constIntValues.hashCode();
        }
    }

    /**
     * 对offset敏感的alias分析，注意：不能给call用！
     */
    public static boolean noAliasSensitiveWithOffset(Value addr1, Value addr2) {
        if (addr1 instanceof GEPInstr gep1 && addr2 instanceof GEPInstr gep2) {
            Value base1 = gep1.getPtrVal();
            Value base2 = gep2.getPtrVal();
            if (base1 == base2) {
                int key1 = encode(gep1.getIndexList(), gep1);
                IndexStatus indexStatus1 = CURR_INDEX_STATUS;
                int key2 = encode(gep2.getIndexList(), gep2);
                IndexStatus indexStatus2 = CURR_INDEX_STATUS;
                if (indexStatus1 == indexStatus2 && key1 != key2) {
                    return true;
                } else {
                    return false;
                }
            }
        }
        return AliasAnalysis.notAlias(addr1, addr2);
    }

}
