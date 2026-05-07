package mid.Optimizer.Loop;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Module;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.HyperParams;
import mid.Optimizer.Optimizer;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.stream.Collectors;

public class LoopAnalyze {
    private final Module module;

    private final HashSet<Loop> loops = new HashSet<>();

    private final HashSet<ScEvValue> scEvValues = new HashSet<>();

    private final HashMap<Loop, HashSet<Loop>> loopTreeChildren = new HashMap<>();

    private final HashSet<Loop> rootLoops = new HashSet<>();

    public LoopAnalyze() {
        this.module = IRManager.getModule();
        for (Function f : module.getDecledFunctions()) {
            for (BasicBlock b : f.getBlocks()) {
                b.setLoopDepth(0);
            }
        }
    }

    public void analyze() {
        loops.clear();
        loopTreeChildren.clear();
        rootLoops.clear();
        //如果a domin b而b->a，则说明存在一个简单循环
        for (Function f : module.getDecledFunctions()) {
            if (f.getBlocks().size() > HyperParams.MAX_BLK_NUM_OPT) {
                // 控制一下循环优化的时间
                continue;
            }
            for (BasicBlock a : f.getBlocks()) {
                Loop loop = new Loop(a);
                for (BasicBlock b : Optimizer.instance().getDominAnalyzer().getDominees(a)) {
                    // 每个循环是以header标识的，即每个循环的header都不同
                    HashSet<BasicBlock> children = Optimizer.instance().getCFG().getChildren(b);
                    if (children == null) {
                        continue;
                    }
                    if (children.contains(a)) {
                        loop.addLatch(b);
                        loops.add(loop);
                    }
                }

                for (BasicBlock block : loop.getBlocksInLoop()) {
                    block.setLoopDepth(block.getLoopDepth() + 1);
                }
            }
        }

        // 识别循环内不变量
        for (Loop loop : loops) {
            HashSet<Value> constantValues = new HashSet<>();
            HashSet<BasicBlock> blocksInLoop = loop.getBlocksInLoop();
            for (BasicBlock b : blocksInLoop) {
                for (Instruction i : b.getInstructionList()) {
                    constantValues.addAll(i.getOperandList().stream().filter(
                                    operand -> (operand instanceof ConstNumber) ||
                                            constantValues.contains(operand) ||
                                            (operand instanceof Instruction instr) &&
                                                    !blocksInLoop.contains(instr.getBlock())).
                            collect(Collectors.toCollection(HashSet::new)));
                    constantValues.remove(i);
                }
            }
        }

        // 构建循环树
        LinkedList<Loop> loopsToSelect = new LinkedList<>(loops);
        while (!loopsToSelect.isEmpty()) {
            Loop loop = loopsToSelect.poll();
            loopTreeChildren.put(loop, new HashSet<>());
            if (rootLoops.size() == 0) {
                rootLoops.add(loop);
                continue;
            }

            boolean flag = false;
            for (Loop rootLoop : rootLoops) {
                if (loop.getBlocksInLoop().contains(rootLoop.getHeader())) {
                    rootLoops.add(loop);
                    rootLoops.remove(rootLoop);
                    loopTreeChildren.get(loop).add(rootLoop);
                    flag = true;
                    break;
                } else if (rootLoop.getBlocksInLoop().contains(loop.getHeader())) {
                    // 寻找最小父节点
                    Loop minParent = rootLoop;
                    while (loopTreeChildren.get(minParent).stream().anyMatch(child ->
                            child.getBlocksInLoop().contains(loop.getHeader()))) {
                        for (Loop childLoop : loopTreeChildren.get(minParent)) {
                            if (childLoop.getBlocksInLoop().contains(loop.getHeader())) {
                                minParent = childLoop;
                                break;
                            }
                        }
                    }

                    HashSet<Loop> children = new HashSet<>(loopTreeChildren.get(minParent));
                    children.removeIf(child -> !loop.getBlocksInLoop().contains(child.getHeader()));
                    loopTreeChildren.get(minParent).removeAll(children);
                    loopTreeChildren.get(minParent).add(loop);
                    loopTreeChildren.put(loop, children);
                    flag = true;
                    break;
                }
            }
            if (!flag) {
                rootLoops.add(loop);
            }
        }
    }

    public HashSet<Loop> getLoops() {
        return loops;
    }

    public HashSet<Loop> getSubLoops(Loop loop) {
        HashSet<Loop> subLoops = new HashSet<>();
        LinkedList<Loop> queue = new LinkedList<>(loopTreeChildren.get(loop));
        while (!queue.isEmpty()) {
            Loop subLoop = queue.poll();
            queue.addAll(loopTreeChildren.get(subLoop));
        }
        return subLoops;
    }

    public void loopMinimize(Loop loop, MinLoop minLoop) {
        loops.remove(loop);
        loops.add(minLoop);
        if (rootLoops.contains(loop)) {
            rootLoops.remove(loop);
            rootLoops.add(minLoop);
        }
        for (Loop itLoop : new HashSet<>(loopTreeChildren.keySet())) {
            if (loopTreeChildren.get(itLoop).contains(loop)) {
                loopTreeChildren.get(itLoop).add(minLoop);
                loopTreeChildren.get(itLoop).remove(loop);
            }
            if (itLoop.equals(loop)) {
                loopTreeChildren.put(minLoop, loopTreeChildren.get(loop));
                loopTreeChildren.remove(loop);
            }
        }
    }

    public ArrayList<Loop> bfsLoops() {
        ArrayList<Loop> loopsFromRoot = new ArrayList<>();
        LinkedList<Loop> queue = new LinkedList<>(rootLoops);
        while (!queue.isEmpty()) {
            Loop loop = queue.poll();
            loopsFromRoot.add(loop);
            queue.addAll(loopTreeChildren.get(loop));
        }
        return loopsFromRoot;
    }

    public HashSet<ScEvValue> getScEvValues() {
        return scEvValues;
    }

    public HashSet<ArrayList<Loop>> getNestedLoops() {
        HashSet<ArrayList<Loop>> nestedLoops = new HashSet<>();
        for (Loop rootLoop : rootLoops) {
            dfsLoopTreeWithPath(rootLoop, new ArrayList<>(), nestedLoops);
        }
        return nestedLoops;
    }

    private void dfsLoopTreeWithPath(Loop loop, ArrayList<Loop> curPath, HashSet<ArrayList<Loop>> paths) {
        curPath.add(loop);
        if (loopTreeChildren.get(loop).size() == 0) {
            paths.add(curPath);
        } else {
            for (Loop childLoop : loopTreeChildren.get(loop)) {
                dfsLoopTreeWithPath(childLoop, new ArrayList<>(curPath), paths);
            }
        }
    }

    public int subLoopCount(Loop loop) {
        return loopTreeChildren.get(loop).size();
    }

    public void removeLoop(Loop loop) {
        loops.remove(loop);
        rootLoops.remove(loop);
        loopTreeChildren.remove(loop);
        for (Loop itLoop : loopTreeChildren.keySet()) {
            loopTreeChildren.get(itLoop).remove(loop);
        }
    }
}
