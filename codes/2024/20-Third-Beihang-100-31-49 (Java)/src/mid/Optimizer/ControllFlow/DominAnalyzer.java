package mid.Optimizer.ControllFlow;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.IRManager;
import mid.Optimizer.Optimizer;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;

public class DominAnalyzer {
    private final HashMap<BasicBlock, HashSet<BasicBlock>> dominers = new HashMap<>();
    private final HashMap<BasicBlock, HashSet<BasicBlock>> dominees = new HashMap<>();
    //基本块的直接支配节点
    private final HashMap<BasicBlock, HashSet<BasicBlock>> iDominees = new HashMap<>();
    //基本块的直接支配者
    private final HashMap<BasicBlock, BasicBlock> iDominers = new HashMap<>();

    private final HashMap<BasicBlock, HashSet<BasicBlock>> dominFrontiers = new HashMap<>();

    private ControlFlowGraph CFG;

    public void analyze(ControlFlowGraph CFG) {
        this.CFG = CFG;
        HashSet<BasicBlock> entrances = new HashSet<>();
        IRManager.getModule().getDecledFunctions().forEach(f -> entrances.add(f.getEntranceBlock()));

        boolean hasChanged;


        for (BasicBlock entry : entrances) {
            hasChanged = true;
            HashSet<BasicBlock> blocks = new HashSet<>(entry.getFunction().getBlocks());
            while (hasChanged) {
                for (BasicBlock bb : blocks) {
                    dominers.putIfAbsent(bb, new HashSet<>(blocks));
                    dominees.putIfAbsent(bb, new HashSet<>());
                }
                hasChanged = bfsDominWithEntry(entry);
            }
        }


        //将支配集转为支配树
        for (BasicBlock entrance : entrances) {
            turnToiDominTree(entrance);
        }

        //计算支配边界
        calDF();
    }

    public void calDF() {
        HashMap<BasicBlock, HashSet<BasicBlock>> bbChildren = CFG.getChildrenGraph();
        BasicBlock nodeX;
        for (BasicBlock nodeA : bbChildren.keySet()) {
            for (BasicBlock nodeB : bbChildren.get(nodeA)) {
                nodeX = nodeA;
                //这里dominees.get(nodeX)不可能是null，至少有一个nodeX，即支配其自己
                while (nodeX.equals(nodeB) || !dominees.get(nodeX).contains(nodeB)) {
                    if (!dominFrontiers.containsKey(nodeX)) {
                        dominFrontiers.put(nodeX, new HashSet<>());
                    }
                    dominFrontiers.get(nodeX).add(nodeB);
                    nodeX = iDominers.get(nodeX);
                    if (nodeX == null) {
                        //如果没有直接支配者，也就没有必要迭代了
                        break;
                    }
                }
            }
        }
    }

    public void analyze(ControlFlowGraph CFG, Function f) {
        this.CFG = CFG;
        BasicBlock entry = f.getEntranceBlock();
        HashSet<BasicBlock> blocks = new HashSet<>(f.getBlocks());

        for (BasicBlock b : f.getBlocks()) {
            dominees.remove(b);
            dominers.remove(b);
            iDominers.remove(b);
            iDominees.remove(b);
        }

        boolean hasChanged = true;
        while (hasChanged) {
            for (BasicBlock bb : blocks) {
                dominers.putIfAbsent(bb, new HashSet<>(blocks));
                dominees.putIfAbsent(bb, new HashSet<>());
            }
            hasChanged = bfsDominWithEntry(entry);
        }

        //将支配集转为支配树
        turnToiDominTree(entry);
    }

    private boolean bfsDominWithEntry(BasicBlock entry) {
        boolean hasChanged = false;


        HashSet<BasicBlock> visited = new HashSet<>();
        HashSet<BasicBlock> blocks = new HashSet<>(entry.getFunction().getBlocks());
        LinkedList<BasicBlock> queue = new LinkedList<>();
        queue.add(entry);

        while (!queue.isEmpty()) {
            BasicBlock bb = queue.poll();
            visited.add(bb);

            //基本块的支配者集合是所有前驱块的dominer的交集加上自身
            HashSet<BasicBlock> parents = CFG.getParents(bb);
            //求出相应的交集
            HashSet<BasicBlock> commonParents = new HashSet<>();

            if (parents != null) {
                commonParents.addAll(blocks);
                for (BasicBlock parent : parents) {
                    //对尚未分析的块不考虑（由于是单入口的程序）
                    commonParents.retainAll(dominers.get(parent));
                    if (commonParents.size() == 0) {
                        break;
                    }
                }
            }
            commonParents.add(bb);

            //检查是否有变化
            for (BasicBlock block : commonParents) {
                if (!dominees.get(block).contains(bb)) {
                    dominees.get(block).add(bb);
                    hasChanged = true;
                }
                if (!dominers.get(bb).contains(block)) {
                    dominers.get(bb).add(block);
                    hasChanged = true;
                }
            }

            ArrayList<BasicBlock> removeBlocks = new ArrayList<>();
            for (BasicBlock block : dominers.get(bb)) {
                if (!commonParents.contains(block)) {
                    hasChanged = true;
                    removeBlocks.add(block);
                }
            }
            removeBlocks.forEach(dominers.get(bb)::remove);
            removeBlocks.forEach(b -> dominees.get(b).remove(bb));

            if (CFG.getChildren(bb) != null) {
                queue.addAll(CFG.getChildren(bb).stream().filter(
                        child -> !(visited.contains(child) || queue.contains(child))).toList());
            }
        }
        return hasChanged;
    }

    private void turnToiDominTree(BasicBlock bb) {
        HashSet<BasicBlock> visited = new HashSet<>();
        LinkedList<BasicBlock> queue = new LinkedList<>();
        //直接支配节点=所有支配节点-(支配节点的所有严格支配节点)
        queue.add(bb);
        while (!queue.isEmpty()) {
            BasicBlock block = queue.poll();
            if (visited.contains(block)) {
                continue;
            }
            visited.add(block);

            HashSet<BasicBlock> iDNodes = new HashSet<>(dominees.get(block));
            for (BasicBlock dominee : dominees.get(block)) {
                if (dominee.equals(block)) {
                    iDNodes.remove(dominee);
                    continue;
                }
                ArrayList<BasicBlock> domineesOfDominee = new ArrayList<>(dominees.get(dominee));
                domineesOfDominee.remove(dominee);
                domineesOfDominee.forEach(iDNodes::remove);
                queue.add(dominee);
            }

            for (BasicBlock iDominee : iDNodes) {
                iDominers.put(iDominee, block);
            }
            iDominees.put(block, iDNodes);
        }
    }

    public HashSet<BasicBlock> getDFOf(BasicBlock block) {
        return dominFrontiers.get(block);
    }

    public HashMap<BasicBlock, HashSet<BasicBlock>> getDominTree() {
        return iDominees;
    }

    public ControlFlowGraph getCFG() {
        return CFG;
    }

    public ArrayList<BasicBlock> bfsDominTreeArray(BasicBlock entry) {
        LinkedList<BasicBlock> queue = new LinkedList<>();
        ArrayList<BasicBlock> retArray = new ArrayList<>();

        queue.add(entry);
        while (!queue.isEmpty()) {
            BasicBlock block = queue.poll();
            retArray.add(block);
            if (iDominees.containsKey(block) && iDominees.get(block) != null) {
                queue.addAll(iDominees.get(block));
            }
        }
        return retArray;
    }

    public boolean canDomin(BasicBlock b1, BasicBlock b2) {
        return dominees.get(b1).contains(b2) || dominers.get(b1).contains(b2);
    }

    public HashMap<BasicBlock, Integer> getDominDepths(Function function) {
        LinkedList<BasicBlock> queue = new LinkedList<>();
        HashMap<BasicBlock, Integer> depths = new HashMap<>();

        queue.add(function.getEntranceBlock());
        depths.put(function.getEntranceBlock(), 0);
        while (!queue.isEmpty()) {
            BasicBlock block = queue.poll();
            if (iDominees.containsKey(block) && iDominees.get(block) != null) {
                queue.addAll(iDominees.get(block));
                for (BasicBlock child : iDominees.get(block)) {
                    depths.put(child, depths.get(block) + 1);
                }
            }
        }
        return depths;
    }

    public BasicBlock LCAInDominTree(BasicBlock b1, BasicBlock b2) {
        //在支配树中查找最小的公共祖先
        //查找b1的所有祖先，在树中，所有节点都只有一个祖先
        if (b1 == null && b2 == null) {
            return null;
        }
        if (b1 == null) {
            return b2;
        }
        if (b2 == null) {
            return b1;
        }
        ArrayList<BasicBlock> ancestors = new ArrayList<>();
        BasicBlock block = b1;
        while (iDominers.containsKey(block)) {
            ancestors.add(block);
            block = iDominers.get(block);
        }
        block = b2;
        while (iDominers.containsKey(block)) {
            if (ancestors.contains(block)) {
                return block;
            }
            block = iDominers.get(block);
        }
        return b1.getFunction().getEntranceBlock();
    }

    public BasicBlock getIDominer(BasicBlock dominee) {
        return iDominers.get(dominee);
    }

    public HashSet<BasicBlock> getDominer(BasicBlock dominee) {
        return dominers.get(dominee);
    }

    public HashSet<BasicBlock> getDominees(BasicBlock dominer) {
        return dominees.get(dominer);
    }

    public void addBlockBetween(BasicBlock prev, BasicBlock next, BasicBlock mid) {
        dominers.put(mid, new HashSet<>(dominers.get(prev)));
        dominees.put(mid, new HashSet<>(dominees.get(prev)));
        dominees.get(mid).remove(prev);
        dominees.get(mid).add(mid);

        for (BasicBlock dominer : dominers.get(prev)) {
            dominees.get(dominer).add(mid);
        }
        for (BasicBlock dominee : dominees.get(prev)) {
            dominers.get(dominee).add(mid);
        }

        iDominees.put(mid, new HashSet<>(iDominees.get(prev)));
        iDominers.put(mid, prev);

        iDominees.get(prev).clear();
        iDominees.get(prev).add(mid);
        for (BasicBlock iDominee : iDominees.get(prev)) {
            iDominers.put(iDominee, mid);
        }

        for (BasicBlock block : dominFrontiers.keySet()) {
            if (dominFrontiers.get(block).contains(next)) {
                dominFrontiers.get(block).remove(next);
                dominFrontiers.get(block).add(mid);
            }
        }
    }
}
