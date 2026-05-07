package pass.ir;

import driver.Config;
import ir.IrInstr;
import ir.IrModule;
import ir.instr.*;
import ir.value.BasicBlock;
import ir.value.Function;
import ir.value.Intermediate;
import pass.Pass;
import pass.utils.LoopInfo;

import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.LinkedList;
import java.util.List;

public class LoopHoist implements Pass.IrPass {
    private List<LoopInfo> loopsToHoist = new LinkedList<>();
    private BasicBlock currentHoistedBlock;

    @Override
    public void run(IrModule module) {
        for (var function : module.getFunctions()) {
            loopsToHoist.clear();
            run(function);
            function.invalidateDomTree();
        }
    }

    private void run(Function function) {
        // 1. Get all loops in order. Inner loops come first.
        for (var topLoop : function.getLoops()) {
            visitLoop(topLoop);
            loopsToHoist.add(topLoop);
        }

        // 2. Hoist all loops.
        for (var loop : loopsToHoist) {
            // 2.1 Create a new block before the loop header.
            currentHoistedBlock = new BasicBlock(loop.getHeaderBlock().getName() + "_hoisted_" + loop.toNameString());
            loop.setHoistedBlock(currentHoistedBlock);
            for (var pred : new LinkedList<>(function.getCFG().getPredecessors(loop.getHeaderBlock()))) {
                if (loop.getLatchBlocks().contains(pred)) {
                    continue;
                }
                // Modify the jump target
                pred.getTerminator().replaceOperand(loop.getHeaderBlock(), currentHoistedBlock);
                // Modify the CFG. If this line is examined to be useless, it can be removed.
                function.getCFG().insertPredecessor(loop.getHeaderBlock(), pred, currentHoistedBlock);
                // Insert in the loop
                LoopInfo visitedLoop = loop.getParentLoop();
                while (visitedLoop != null) {
                    visitedLoop.insertBefore(loop.getHeaderBlock(), currentHoistedBlock);
                    visitedLoop = visitedLoop.getParentLoop();
                }
                // Modify the phi nodes sources
                for (var instr : loop.getHeaderBlock().getInstrs()) {
                    if (instr instanceof PhiInstr phi) {
                        phi.replaceBasicBlock(pred.getName(), currentHoistedBlock);
                    } else {
                        break;
                    }
                }
            }
            // 2.2 Check all expressions to filter those hoist-able.
            LinkedHashSet<IrInstr> marked = new LinkedHashSet<>();
            HashSet<String> valuePointInner = new HashSet<>();
            for (var block : loop.getBlocks()) {
                for (var instr : block.getInstrs()) {
                    var lhs = instr.getInitialValue();
                    if (lhs instanceof Intermediate intermediate) {
                        valuePointInner.add(intermediate.getName());
                    }
                }
            }
            // System.err.println(valuePointInner);
            boolean changed = true;
            while (changed) {
                changed = false;
                for (var block : loop.getBlocks()) {
                    var iter = block.getInstrs().iterator();
                    while (iter.hasNext()) {
                        var instr = iter.next();
                        if ((instr instanceof TermInstr /* && ((TermInstr) instr).op != IrInstr.OpCode.CALL */)  // 函数调用不提出来
                                || instr instanceof AllocaInstr || instr instanceof LoadInstr || instr instanceof CommentInstr
                                || instr instanceof PhiInstr || instr instanceof StoreInstr) {
                            continue;
                        }
                        boolean allInvariant = true;
                        for (var operand : instr.getOperands()) {
                            if (instr.getInitialValue() != null && instr.getInitialValue().equals(operand)) {
                                continue;
                            }
                            if (operand instanceof Intermediate intermediate) {
                                if (valuePointInner.contains(intermediate.getName())) {
                                    allInvariant = false;
                                    break;
                                }
                            }
                        }
                        if (allInvariant && !marked.contains(instr)) {
                            // 2.3 Move hoist-able expressions to the new block.
                            marked.add(instr);
                            iter.remove();
                            currentHoistedBlock.insertTailInstr(instr);
                            if (Config.DebugSettings.displayDetailedLoopHoist) {
                                System.err.println("Move " + instr + " to " + currentHoistedBlock.getName());
                            }
                            if (instr.getInitialValue() instanceof Intermediate intermediateLHS) {
                                valuePointInner.remove(intermediateLHS.getName());
                                loop.addInvariant(intermediateLHS);
                            }
                            changed = true;
                        }
                    }
                }
            }
            // 2.4 Add the new block to the function.
            currentHoistedBlock.insertTailInstr(new TermInstr(loop.getHeaderBlock(), IrInstr.OpCode.BR));
            function.addBasicBlock(currentHoistedBlock);
        }
    }

    private void visitLoop(LoopInfo loop) {
        for (var subLoop : loop.getSubLoops()) {
            visitLoop(subLoop);
            loopsToHoist.add(subLoop);
        }
    }

    @Override
    public String getName() {
        return "loop-hoist";
    }
}
