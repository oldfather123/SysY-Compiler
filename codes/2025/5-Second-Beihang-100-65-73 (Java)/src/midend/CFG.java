package midend;

import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.*;

/**
 * 这个类主要用于构建支配关系（支配、直接支配、支配边界）
 * 我不太好说这个类到底是否应该被称为 CFG，毕竟构建 pre 和 suc 的工作在跳转指令里已经做完了
 */
public class CFG {
    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            executeOnFunc(function);
        }
    }

    public static void executeOnFunc(Function function) {
        //            preSucCheck(function);
        initCFGInfo(function);
        removeDeadBlock(function);
        makeDomSet(function);
        makeIDomSet(function);
        makeDF(function);
        FunctionInfoCollector.getFuncInfo(function).setDomBfsOrder(getDomBfsOrder(function));
        function.computeDomTreeChildren();
//            preSucCheck(function);
    }

    private static void initCFGInfo(Function function) {
        for (BasicBlock basicBlock : function.getBasicBlockList()) {
            basicBlock.initCFGInfo();
        }
    }

    /**
     * 构建支配关系之前应该删除不可达的块，避免 dfs 和顺序遍历找到的块不一样
     */
    private static void removeDeadBlock(Function function) {
        // 从第二个块开始，因为入口块没有前驱但是显然不应该被删除
        boolean removed;
        do {
            removed = false;
            List<BasicBlock> RPO = getReversePostOrder(function);
            HashSet<BasicBlock> used = new HashSet<>(RPO);
            for (BasicBlock bb : function.getBasicBlockList()) {
                if (!used.contains(bb)) {
                    bb.removeFromList();
                    removed = true;
                }
            }
        } while (removed);
    }

    private static void makeDF(Function function) {
        for (BasicBlock blk1 : function.getBasicBlockList()) {
            HashSet<BasicBlock> DF = blk1.getDominanceFrontier();
            for (BasicBlock blk2 : function.getBasicBlockList()) {
                for (BasicBlock tmp : blk2.getPres()) {
                    if (blk1.getDomSet().contains(tmp) &&
                            (blk1.equals(blk2) || !blk1.getDomSet().contains(blk2))) {
                        DF.add(blk2);
                        break;
                    }
                }
            }
        }
    }

    /**
     * 对于某一个基本块 s，其支配一个基本块 u，如果 s 支配的所有基本块都不支配 u，则可以知道 s 直接支配 u
     */
    private static void makeIDomSet(Function function) {
        for (BasicBlock curBlock : function.getBasicBlockList()) {
            curBlock.initIDomSet();
            Set<BasicBlock> idoms = curBlock.getIDomSet();
            for (BasicBlock u : curBlock.getDomSet()) {
                if (u == curBlock) {
                    continue;
                }
                boolean flag = true;
                for (BasicBlock otherBlk : curBlock.getDomSet()) {
                    if (otherBlk == u || otherBlk == curBlock) {
                        continue;
                    }
                    if (otherBlk.getDomSet().contains(u)) {
                        flag = false;
                        break;
                    }
                }
                if (flag) {
                    u.setIDom(curBlock);
//                    System.out.println(curBlock + " -doms> " + u + "  u dom lv = " + u.getDomLV());
                    idoms.add(u);
                }
            }
        }

        int depth = 0;
        Queue<BasicBlock> workSet = new LinkedList<>();
        HashSet<BasicBlock> visited = new HashSet<>();
        workSet.add(function.getFirstBlk());
        while (!workSet.isEmpty()) {
            Queue<BasicBlock> next = new LinkedList<>();
            for (BasicBlock blk : workSet) {
                if (!visited.contains(blk)) {
                    visited.add(blk);
                    blk.setDomLV(depth);
                    next.addAll(blk.getIDomSet());
                }
            }
            workSet = next;
            depth++;
        }
    }

    /**
     * 数据流迭代法求解一个函数中所有基本块的支配集合（当前 bb 支配别的块）
     */
    private static void makeDomSet(Function function) {
        List<BasicBlock> RPO = getReversePostOrder(function);
        Map<BasicBlock, Set<BasicBlock>> block2dominatedBy = new HashMap<>();

        for (BasicBlock block : RPO) {
            block.initDomSet();
            if (block.isEntry()) {
                Set<BasicBlock> doms = new HashSet<>();
                doms.add(block);
                block2dominatedBy.put(block, doms);
            } else {
                block2dominatedBy.put(block, new HashSet<>(RPO)); // 初始被所有块支配
            }
        }

        boolean changed = true;
        while (changed) {
            changed = false;
            for (BasicBlock block : RPO) {
                if (block.isEntry()) continue;

                Set<BasicBlock> newDoms = null;
                for (BasicBlock pred : block.getPres()) {
                    if (newDoms == null) {
                        newDoms = new HashSet<>(block2dominatedBy.get(pred));
                    } else {
                        newDoms.retainAll(block2dominatedBy.get(pred));
                    }
                }
                if (newDoms != null) {
                    newDoms.add(block);
                } else {
                    throw new RuntimeException("newDoms is null");
                }

                if (!newDoms.equals(block2dominatedBy.get(block))) {
                    block2dominatedBy.put(block, newDoms);
                    changed = true;
                }
            }
        }

        for (BasicBlock block : RPO) {
            for (BasicBlock dom : block2dominatedBy.get(block)) {
                dom.getDomSet().add(block);
            }
        }
    }


    /**
     * 构建传入函数中基本块的逆后序序列，方便后续进行数据流迭代
     */
    public static List<BasicBlock> getReversePostOrder(Function function) {
        List<BasicBlock> postOrder = new ArrayList<>();
        BasicBlock basicBlock = function.getFirstBlk();
        dfsBlk4postOrder(basicBlock, postOrder, new HashSet<>());
        Collections.reverse(postOrder);
        return postOrder;
    }

    private static void dfsBlk4postOrder(BasicBlock basicBlock, List<BasicBlock> postOrder,
                                         Set<BasicBlock> visited) {
        visited.add(basicBlock);
        for (BasicBlock suc : basicBlock.getSucs()) {
            if (!visited.contains(suc)) {
                dfsBlk4postOrder(suc, postOrder, visited);
            }
        }
        postOrder.add(basicBlock);
    }


    public static List<BasicBlock> getDomPostOrder(Function function) {
        List<BasicBlock> postOrder = new ArrayList<>();
        Set<BasicBlock> visited = new HashSet<>();
        BasicBlock entry = function.getFirstBlk();  // 支配树根节点
        dfsDomPostOrder(entry, postOrder, visited);
        return postOrder;
    }

    private static void dfsDomPostOrder(BasicBlock block, List<BasicBlock> postOrder, Set<BasicBlock> visited) {
        visited.add(block);
        for (BasicBlock child : block.getIDomSet()) {  // 支配树子节点
            if (!visited.contains(child)) {
                dfsDomPostOrder(child, postOrder, visited);
            }
        }
        postOrder.add(block);
    }

    public static List<BasicBlock> getDomBfsOrder(Function function) {
        List<BasicBlock> result = new ArrayList<>();
        Queue<BasicBlock> queue = new LinkedList<>();
        BasicBlock root = function.getFirstBlk();
        queue.offer(root);
        while (!queue.isEmpty()) {
            BasicBlock cur = queue.poll();
            result.add(cur);
            for (BasicBlock child : cur.getIDomSet()) {
                queue.offer(child);
            }
        }
        return result;
    }

    public static void preSucCheck(Function function) {
        for (BasicBlock blk : function.getBasicBlockList()) {
            for (BasicBlock suc : blk.getSucs()) {
                if (!blk.getLastInstr().getUsedList().contains(suc)) {
                    throw new RuntimeException("preSucCheck failed");
                }
                if (!suc.getPres().contains(blk)) {
                    throw new RuntimeException("preSucCheck failed");
                }
            }
        }
    }
}
