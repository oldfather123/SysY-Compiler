package pass.utils;

import ir.value.BasicBlock;
import ir.value.Intermediate;
import utils.RefCell;

import java.util.*;

/**
 * <img src="https://llvm.org/docs/_images/loop-terminology.svg" />
 */
public class LoopInfo {
    private static int globalId = 0;
    private final int id = ++globalId;
    private final BasicBlock headerBlock;
    private final RefCell<BasicBlock> hoistedBlock = new RefCell<>(null);
    private LoopInfo parentLoop;
    private final List<LoopInfo> subLoops = new ArrayList<>();
    private final List<BasicBlock> blocks = new LinkedList<>();
    private final List<BasicBlock> exitBlocks = new LinkedList<>();
    private final List<BasicBlock> exitingBlocks = new LinkedList<>();
    private final List<BasicBlock> latchBlocks = new LinkedList<>();
    private final List<Intermediate> invariants = new LinkedList<>();

    public List<BasicBlock> getLatchBlocks() {
        return latchBlocks;
    }

    public LoopInfo(BasicBlock header) {
        this.headerBlock = header;
        blocks.add(header);
        this.parentLoop = null;
    }

    public BasicBlock getHeaderBlock() {
        return headerBlock;
    }

    public boolean hasParent() {
        return parentLoop != null;
    }

    public LoopInfo getParentLoop() {
        return parentLoop;
    }

    public void setParentLoop(LoopInfo parentLoop) {
        this.parentLoop = parentLoop;
    }

    public void addSubLoop(LoopInfo subLoop) {
        subLoops.add(subLoop);
    }

    public void reverseSubLoop() {
        Collections.reverse(subLoops);
    }

    public void addBlock(BasicBlock bb) {
        blocks.add(bb);
    }

    public void reverseBlock() {
        BasicBlock first = blocks.remove(0);
        Collections.reverse(blocks);
        blocks.add(0, first);
    }

    public List<BasicBlock> getBlocks() {
        return blocks;
    }

    public List<LoopInfo> getSubLoops() {
        return subLoops;
    }

    public void addExitingBlock(BasicBlock bb) {
        exitingBlocks.add(bb);
    }

    public void addExitBlock(BasicBlock bb) {
        exitBlocks.add(bb);
    }

    public void addLatchBlock(BasicBlock latchBb) {
        latchBlocks.add(latchBb);
    }

    public String toNameString() {
        return "loop_" + id;
    }

    public void addInvariant(Intermediate invariant) {
        invariants.add(invariant);
    }

    public List<Intermediate> getInvariants() {
        return invariants;
    }

    public boolean isSimple(ControlFlowGraph cfg) {
        // 1. Only two predecessors
        if (cfg.getPredecessors(headerBlock).size() != 2) {
            return false;
        }
        // 2. Only one latch block
        if (latchBlocks.size() != 1) {
            return false;
        }
        // 3. Only one exit block
        if (exitBlocks.size() != 1) {
            return false;
        }
        // 4. Only one exiting block
        for (var exiting : exitingBlocks) {
            if (exiting != headerBlock) {
                return false;
            }
        }
        return true;
    }

    public List<BasicBlock> getLevelOrder() {
        List<BasicBlock> levelOrder = new ArrayList<>();
        Queue<BasicBlock> queue = new LinkedList<>();
        queue.add(headerBlock);
        while (!queue.isEmpty()) {
            BasicBlock bb = queue.poll();
            levelOrder.add(bb);
            for (var succ : bb.getSuccessors()) {
                // FIXME: Perf needed
                if (!queue.contains(succ) && !levelOrder.contains(succ) && blocks.contains(succ)) {
                    queue.add(succ);
                }
            }
        }
        return levelOrder;
    }

    @Override
    public String toString() {
        return "Loop_" + id + ": {" +
                "parent=" + (parentLoop == null ? "null" : parentLoop.toNameString()) +
                ", subLoops=" + subLoops.stream().map(loop -> "Loop#" + loop.id).toList() +
                ", header=" + headerBlock +
                ", exiting=" + exitingBlocks +
                ", exit=" + exitBlocks +
                ", latch=" + latchBlocks +
                ", blocks=" + blocks +
                '}';
    }

    public void insertBefore(BasicBlock block, BasicBlock newBlock) {
        int index = blocks.indexOf(block);
        blocks.add(index, newBlock);
    }

    public void setHoistedBlock(BasicBlock hoistedBlock) {
        this.hoistedBlock.set(hoistedBlock);
    }

    public BasicBlock getHoistedBlock() {
        return hoistedBlock.getNotNull();
    }
}
