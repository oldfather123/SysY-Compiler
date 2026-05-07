package pass.utils;

import ir.instr.CommentInstr;
import ir.value.BasicBlock;
import ir.value.Function;

import java.util.*;

public class ControlFlowGraph {
    private final List<BasicBlock> blocks = new ArrayList<>();
    private BasicBlock entry;
    private final Map<BasicBlock, List<BasicBlock>> predecessors = new HashMap<>();
    private final Map<BasicBlock, List<BasicBlock>> successors = new HashMap<>();

    public static ControlFlowGraph fromFunction(Function function) {
        ControlFlowGraph cfg = new ControlFlowGraph();
        cfg.entry = function.getBasicBlocks().get(0);

        HashSet<BasicBlock> visited = new HashSet<>();
        LinkedList<BasicBlock> queue = new LinkedList<>();
        queue.add(cfg.entry);

        while (!queue.isEmpty()) {
            BasicBlock block = queue.poll();
            if (visited.contains(block)) {
                continue;
            }
            visited.add(block);
            cfg.blocks.add(block);
            cfg.predecessors.computeIfAbsent(block, k -> new ArrayList<>());
            cfg.successors.put(block, new ArrayList<>());
            for (BasicBlock successor : block.getSuccessors()) {
                cfg.predecessors.computeIfAbsent(successor, k -> new ArrayList<>()).add(block);
                cfg.successors.get(block).add(successor);
                queue.add(successor);
            }
        }

        return cfg;
    }

    public void insertPredecessor(BasicBlock block, BasicBlock pred, BasicBlock newBlock) {
        predecessors.get(block).remove(pred);
        predecessors.computeIfAbsent(newBlock, k -> new ArrayList<>()).add(pred);
        predecessors.get(block).add(newBlock);
        successors.get(pred).remove(block);
        successors.get(pred).add(newBlock);
        successors.computeIfAbsent(newBlock, k -> new ArrayList<>()).add(block);
    }

    public void applyBlock(Function function) {
        blocks.removeIf(b -> !b.equals(entry) && getPredecessors(b).isEmpty());
        function.resetBasicBlock((ArrayList<BasicBlock>) blocks);
    }

    public BasicBlock getEntry() {
        return entry;
    }

    public List<BasicBlock> getSuccessors(BasicBlock block) {
        return successors.get(block);
    }

    public List<BasicBlock> getPredecessors(BasicBlock block) {
        return predecessors.get(block);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("Control Flow Graph").append('\n');
        for (var block : blocks) {
            sb.append("  ").append(block).append(":\n");
            sb.append("  - Successor: ").append(getSuccessors(block)).append('\n');
            sb.append("  - Predecessor: ").append(getPredecessors(block)).append('\n');
        }
        return sb.toString();
    }

    public void filterComment() {
        for (var block : blocks) {
            block.getInstrs().removeIf(i -> i instanceof CommentInstr);
        }
    }
}
