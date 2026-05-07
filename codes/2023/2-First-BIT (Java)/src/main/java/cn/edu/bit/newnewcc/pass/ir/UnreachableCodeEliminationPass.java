package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.exception.ValueBeingUsedException;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.instruction.UnreachableInst;

import java.util.*;
import java.util.function.Consumer;

/**
 * 删除不可达代码
 * <p>
 * 该Pass的功能是DeadCodeEliminationPass的子集
 */
public class UnreachableCodeEliminationPass {
    private UnreachableCodeEliminationPass() {
    }

    private static void runOnFunction(Function function) {
        var reachableBlocks = getReachableBlocks(function);
        var unreachableBlocks = getUnreachableBlocks(function, reachableBlocks);
        removeBlocks(unreachableBlocks);
    }

    private static void removeBlocks(Collection<BasicBlock> removingBlocks) {
        // 将该块从其出口中移除
        for (BasicBlock removingBlock : removingBlocks) {
            for (BasicBlock exitBlock : removingBlock.getExitBlocks()) {
                exitBlock.removeEntryFromPhi(removingBlock);
            }
            removingBlock.setTerminateInstruction(new UnreachableInst());
        }
        // 现在，所有块都没有入口、出口，也不会被使用
        for (BasicBlock removingBlock : removingBlocks) {
            // 入口数量为0会被下方条件检查，出口数量为0是上方代码实现的
            if (removingBlock.getUsages().size() > 0) {
                throw new ValueBeingUsedException();
            }
        }
        // 先清理操作数引用
        for (BasicBlock removingBlock : removingBlocks) {
            for (Instruction instruction : removingBlock.getInstructions()) {
                instruction.clearOperands();
            }
        }
        // 而后直接移除基本块（无需移除基本块中的语句，他们已经与外界无关了）
        for (BasicBlock removingBlock : removingBlocks) {
            var function = removingBlock.getFunction();
            if (function != null) {
                function.removeBasicBlock(removingBlock);
            }
        }
    }

    private static Collection<BasicBlock> getUnreachableBlocks(Function function, Collection<BasicBlock> reachableBlocks) {
        var removeBlockList = new ArrayList<BasicBlock>();
        for (BasicBlock block : function.getBasicBlocks()) {
            if (!reachableBlocks.contains(block)) {
                removeBlockList.add(block);
            }
        }
        return removeBlockList;
    }

    private static Set<BasicBlock> getReachableBlocks(Function function) {
        var visitedBlocks = new HashSet<BasicBlock>();
        Queue<BasicBlock> queue = new ArrayDeque<>();
        Consumer<BasicBlock> pushBlockToQueue = (BasicBlock basicBlock) -> {
            if (visitedBlocks.contains(basicBlock)) return;
            visitedBlocks.add(basicBlock);
            queue.add(basicBlock);
        };
        pushBlockToQueue.accept(function.getEntryBasicBlock());
        while (!queue.isEmpty()) {
            var block = queue.remove();
            for (BasicBlock exitBlock : block.getExitBlocks()) {
                pushBlockToQueue.accept(exitBlock);
            }
        }
        return visitedBlocks;
    }

    public static void runOnModule(Module module) {
        module.getFunctions().forEach(UnreachableCodeEliminationPass::runOnFunction);
    }

}
