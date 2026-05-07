package IR;

import IR.IRValueRef.IRBaseBlockRef;
import IR.IRValueRef.IRFunctionBlockRef;
import backend.MemoryRegisterAlloc;

import java.util.*;
/**
 * 该类是用于求支配边界，使phi指令引入
 */
public class DomAnalysis {
    //支配：n1支配n2当且仅当所有从入口节点到n2的路径中都包含n1
    private final IRFunctionBlockRef functionBlock;
    private final List<IRBaseBlockRef> basicBlocks;
    //支配关系
    private final Map<IRBaseBlockRef, Set<IRBaseBlockRef>> dominators;
    //直接支配者
    private final Map<IRBaseBlockRef, IRBaseBlockRef> immediateDominators;
    //支配边界
    private final Map<IRBaseBlockRef, Set<IRBaseBlockRef>> dominanceFrontiers;
    //前驱后驱
    MemoryRegisterAlloc memoryRegisterAlloc = new MemoryRegisterAlloc();
    Map<IRBaseBlockRef, List<IRBaseBlockRef>> predecessors;
    public DomAnalysis(IRFunctionBlockRef functionBlock) {
        this.functionBlock = functionBlock;
        this.basicBlocks = functionBlock.getBaseBlocks();
        this.dominators = new HashMap<>();
        this.immediateDominators = new HashMap<>();
        this.dominanceFrontiers = new HashMap<>();
        predecessors = computePredecessors(functionBlock);
        //初始化每个块支配者为全部块
        for (IRBaseBlockRef block : basicBlocks) {
            dominators.put(block, new HashSet<>(basicBlocks));
        }
        this.run();
    }

    public void run() {
        computeDominators();
        computeImmediateDominators();
        computeDominanceFrontiers();
    }

    //求支配关系，数据流迭代O(n^2)复杂度
    private void computeDominators() {
        //进入块
        IRBaseBlockRef entryBlock = functionBlock.getBaseBlocks().get(0);
        dominators.get(entryBlock).clear();
        dominators.get(entryBlock).add(entryBlock);

        boolean changed;
        do {
            changed = false;
            // 按照反向后序遍历处理所有节点
            List<IRBaseBlockRef> reversePostOrder = getReversePostOrder(functionBlock);
            for (IRBaseBlockRef block : reversePostOrder) {
                if (block == entryBlock) continue;
                // 计算新的支配者集合
                Set<IRBaseBlockRef> newDominators = new HashSet<>(dominators.get(predecessors.get(block).get(0)));
                for (IRBaseBlockRef predBlock : predecessors.get(block)) {
                    newDominators = intersection(newDominators, dominators.get(predBlock));
                }
                newDominators.add(block);

                if (!newDominators.equals(dominators.get(block))) {
                    dominators.put(block, newDominators);
                    changed = true;
                }
            }
        } while (changed);
    }

    private Set<IRBaseBlockRef> intersection(Set<IRBaseBlockRef> set1, Set<IRBaseBlockRef> set2) {
        Set<IRBaseBlockRef> result = new HashSet<>();
        Map<IRBaseBlockRef, Boolean> map = new HashMap<>();
        for (IRBaseBlockRef element : set2) {
            map.put(element, true);
        }
        for (IRBaseBlockRef element : set1) {
            if (map.containsKey(element)) {
                result.add(element);
            }
        }
        return result;
    }

    private List<IRBaseBlockRef> getReversePostOrder(IRFunctionBlockRef functionBlock) {
        List<IRBaseBlockRef> postOrder = new ArrayList<>();
        Set<IRBaseBlockRef> visited = new HashSet<>();
        IRBaseBlockRef entryBlock = functionBlock.getBaseBlocks().get(0);
        // 使用递归DFS来进行后序遍历
        dfs(entryBlock, visited, postOrder);
        // 反转后序遍历结果，得到反向后序遍历
        Collections.reverse(postOrder);
        return postOrder;
    }

    private void dfs(IRBaseBlockRef block, Set<IRBaseBlockRef> visited, List<IRBaseBlockRef> postOrder) {
        if (!visited.contains(block)) {
            visited.add(block);
            // 遍历所有后继节点
            for (IRBaseBlockRef succ : memoryRegisterAlloc.getImmediateSuccessors(block)) {
                dfs(succ, visited, postOrder);
            }
            postOrder.add(block);
        }
    }

    // 计算直接支配者
    private void computeImmediateDominators() {
        for (IRBaseBlockRef block : basicBlocks) {
            if (block == functionBlock.getBaseBlocks().get(0)) continue;
            IRBaseBlockRef idom = null;
            for (IRBaseBlockRef dom : dominators.get(block)) {
                if (dom == block) continue;
                HashSet<IRBaseBlockRef> tmpDomSet = new HashSet<>(dominators.get(block));
                tmpDomSet.remove(block);
                //tmpDomSet是去掉自己的所有支配者
                //如果支配者A中，某支配者的所有支配者是A去掉自己的支配者集合，则最近？
                if (idom == null && dominators.get(dom).equals(tmpDomSet)) {
                    idom = dom;
                }
            }
            immediateDominators.put(block, idom);
        }
    }


    // 计算支配边界
    private void computeDominanceFrontiers() {
        // 初始化每个基本块的支配边界集合
        for (IRBaseBlockRef block : basicBlocks) {
            dominanceFrontiers.put(block, new HashSet<>());
        }
        // 遍历每个基本块
        for (IRBaseBlockRef block : basicBlocks) {
            for (IRBaseBlockRef runner : predecessors.get(block)) {
                while (runner != null && !dominators.get(block).contains(runner)) {
                    dominanceFrontiers.get(runner).add(block);
                    runner = immediateDominators.get(runner);
                }
            }
        }
    }

    public Map<IRBaseBlockRef, List<IRBaseBlockRef>> computePredecessors(IRFunctionBlockRef functionBlock) {
        Map<IRBaseBlockRef, List<IRBaseBlockRef>> predecessors = new HashMap<>();
        // 初始化前驱列表
        for (IRBaseBlockRef block : functionBlock.getBaseBlocks()) {
            predecessors.put(block, new ArrayList<>());
        }
        // 遍历每个基本块，利用后继列表填充前驱列表
        for (IRBaseBlockRef block : functionBlock.getBaseBlocks()) {
            for (IRBaseBlockRef succ : memoryRegisterAlloc.getImmediateSuccessors(block)) {
                predecessors.get(succ).add(block);
            }
        }
        return predecessors;
    }

    // 判断x是否支配b
    private boolean dominates(IRBaseBlockRef x, IRBaseBlockRef b) {
        return dominators.get(b).contains(x);
    }

    // 返回该函数所有块的支配关系
    public Map<IRBaseBlockRef, Set<IRBaseBlockRef>> getDominators() {
        return dominators;
    }

    // 返回该函数所有块的支配边界
    public Map<IRBaseBlockRef, Set<IRBaseBlockRef>> getDominanceFrontiers() {
        return dominanceFrontiers;
    }

    // 返回该函数所有块的直接支配者
    public Map<IRBaseBlockRef, IRBaseBlockRef> getImmediateDominators() {
        return immediateDominators;
    }
}