package pass.ir;

import ir.IrModule;
import ir.instr.MoveInstr;
import ir.instr.PhiInstr;
import ir.value.BasicBlock;
import ir.value.Function;
import pass.Pass;
import pass.utils.ControlFlowGraph;

import java.util.Deque;
import java.util.HashSet;
import java.util.LinkedList;


public class MergeBasicBlock implements Pass.IrPass {
    @Override
    public void run(IrModule module) {
        for (Function func : module.getFunctions()) {
            func.invalidateDomTree();
            func.invalidateLoopInfo();
            mergeBasicBlock(func);
        }
    }

    private static void mergeBasicBlock(Function func) {
        ControlFlowGraph cfg = ControlFlowGraph.fromFunction(func);
        cfg.filterComment();

        HashSet<BasicBlock> visited = new HashSet<>();
        Deque<BasicBlock> queue = new LinkedList<>();
        queue.add(cfg.getEntry());
        while (!queue.isEmpty()) {
            BasicBlock from = queue.poll();

            if (visited.contains(from)) {
                continue;
            }

            visited.add(from);

            if (cfg.getSuccessors(from).size() == 1) {
                BasicBlock to = cfg.getSuccessors(from).get(0);
                if (cfg.getPredecessors(to).size() == 1) {
                    // Merge the instruction of `from` to `to`
                    from.getInstrs().removeLast();
                    var iterTo = to.getInstrs().iterator();
                    while (iterTo.hasNext()) {
                        var instr = iterTo.next();
                        if (instr instanceof PhiInstr phi) {
                            from.getInstrs().add(new MoveInstr(phi.getInitialValue(), phi.getDependentValues().get(1)));
                            iterTo.remove();
                        } else {
                            break;  // PHI are always at the beginning of a block
                        }
                    }
                    from.getInstrs().addAll(to.getInstrs());
                    // Change the control flow
                    cfg.getSuccessors(from).clear();
                    cfg.getSuccessors(from).addAll(cfg.getSuccessors(to));
                    for (BasicBlock successor : cfg.getSuccessors(to)) {
                        cfg.getPredecessors(successor).remove(to);
                        cfg.getPredecessors(successor).add(from);
                        // Modify the PHI instruction
                        for (var instr : successor.getInstrs()) {
                            if (instr instanceof PhiInstr phi) {
                                phi.replaceBasicBlock(to.getName(), from);
                            } else {
                                break;
                            }
                        }
                    }
                    // Continue the iter
                    visited.remove(from);
                    queue.addFirst(from);
                    // Delete the block
                    cfg.getPredecessors(to).clear();
                } else {
                    queue.add(to);
                }
            } else {
                queue.addAll(cfg.getSuccessors(from));
            }
        }

        cfg.applyBlock(func);
    }

    @Override
    public String getName() {
        return "merge-basic-block";
    }
}
