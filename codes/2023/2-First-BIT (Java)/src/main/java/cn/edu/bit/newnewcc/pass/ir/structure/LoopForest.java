package cn.edu.bit.newnewcc.pass.ir.structure;

import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;

import java.util.*;

/**
 * 循环森林
 * <p>
 * 每个函数内包含若干个循环树，这些树构成一个循环森林
 */
public class LoopForest implements Iterable<Loop> {

    /**
     * 构建过程中临时存储的全局信息
     * <p>
     * 将在构建完成后被自动释放
     */
    private static class Builder {
        public final DomTree domTree;

        public Builder(Function function) {
            domTree = DomTree.buildOver(function);
        }
    }

    /**
     * 所有循环树的根
     */
    private final Set<Loop> rootLoops = new HashSet<>();

    /**
     * 基本块到循环的映射关系
     */
    private final Map<BasicBlock, Loop> basicBlockLoopMap = new HashMap<>();

    /**
     * 基于一个函数构建其中基本块形成的循环森林
     *
     * @param function 函数
     */
    private LoopForest(Function function) {
        var builder = new Builder(function);
        dfsFindLoops(builder, function.getEntryBasicBlock());
        for (Loop loop : basicBlockLoopMap.values()) {
            if (loop.getParentLoop() == null) {
                rootLoops.add(loop);
            }
        }
        for (Loop rootLoop : rootLoops) {
            rootLoop.__setLoopDepth__(0);
            dfsSetLoopDepth(rootLoop);
        }
    }

    private void dfsSetLoopDepth(Loop currentLoop) {
        for (Loop subLoop : currentLoop.getSubLoops()) {
            subLoop.__setLoopDepth__(currentLoop.getLoopDepth() + 1);
            dfsSetLoopDepth(subLoop);
        }
    }

    private void dfsFindLoops(Builder builder, BasicBlock basicBlock) {
        for (BasicBlock domSon : builder.domTree.getDomSons(basicBlock)) {
            dfsFindLoops(builder, domSon);
        }
        boolean isLoopHeader = false;
        Set<Loop> subLoops = new HashSet<>();
        Set<BasicBlock> loopBasicBlocks = new HashSet<>();
        loopBasicBlocks.add(basicBlock);
        for (BasicBlock entryBlock : basicBlock.getEntryBlocks()) {
            if (builder.domTree.doesAlphaDominatesBeta(basicBlock, entryBlock)) {
                isLoopHeader = true;
                Queue<BasicBlock> queue = new ArrayDeque<>();
                queue.add(entryBlock);
                while (!queue.isEmpty()) {
                    var u = queue.remove();
                    if (basicBlockLoopMap.containsKey(u)) {
                        var cloop = basicBlockLoopMap.get(u);
                        while (cloop.getParentLoop() != null) {
                            cloop = cloop.getParentLoop();
                        }
                        if (!subLoops.contains(cloop)) {
                            subLoops.add(cloop);
                            queue.addAll(cloop.getHeaderBasicBlock().getEntryBlocks());
                        }
                    } else {
                        if (!loopBasicBlocks.contains(u)) {
                            loopBasicBlocks.add(u);
                            queue.addAll(u.getEntryBlocks());
                        }
                    }
                }
            }
        }
        if (isLoopHeader) {
            var loop = new Loop(basicBlock, subLoops, loopBasicBlocks);
            subLoops.forEach(subLoop -> subLoop.__setParentLoop__(loop));
            loopBasicBlocks.forEach(loopBasicBlock -> basicBlockLoopMap.put(loopBasicBlock, loop));
            basicBlockLoopMap.put(basicBlock, loop);
        }
    }

    /**
     * @return 所有循环树的根循环列表（只读）
     */
    public Collection<Loop> getRootLoops() {
        return Collections.unmodifiableSet(rootLoops);
    }

    /**
     * @return 基本块到循环的映射（只读）
     */
    public Map<BasicBlock, Loop> getBasicBlockLoopMap() {
        return Collections.unmodifiableMap(basicBlockLoopMap);
    }

    @Override
    public Iterator<Loop> iterator() {
        return getRootLoops().iterator();
    }

    /**
     * 在函数的基础上建立一个循环森林 <br>
     * 由于循环标准化的需要，可能对传入的函数产生副作用！！！ <br>
     *
     * @param function 函数（可能会被修改！！！）
     * @return 循环森林
     */
    public static LoopForest buildOver(Function function) {
        return new LoopForest(function);
    }

}
