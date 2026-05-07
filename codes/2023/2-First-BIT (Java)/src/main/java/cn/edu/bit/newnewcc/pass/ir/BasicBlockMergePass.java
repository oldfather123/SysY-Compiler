package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.exception.IllegalStateException;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.instruction.PhiInst;

public class BasicBlockMergePass {

    private static boolean runOnBasicBlock(BasicBlock prevBlock) {
        if (prevBlock.getFunction() == null) return false;
        boolean changed = false;
        while (prevBlock.getExitBlocks().size() == 1) {
            var nextBlock = prevBlock.getExitBlocks().iterator().next();
            if (nextBlock.getEntryBlocks().size() != 1) break;
            // 处理前导语句
            for (Instruction leadingInstruction : nextBlock.getLeadingInstructions()) {
                if (leadingInstruction instanceof PhiInst phiInst) {
                    phiInst.replaceAllUsageTo(phiInst.getValue(prevBlock));
                    phiInst.waste();
                } else {
                    // 非函数入口块应当只包含 Phi 语句
                    // 函数入口块不会有任何入口，不会成为 next block
                    throw new IllegalStateException("Non-entry basic block contains non-phi instruction.");
                }
            }
            // 处理主体语句
            for (Instruction mainInstruction : nextBlock.getMainInstructions()) {
                mainInstruction.removeFromBasicBlock();
                prevBlock.addInstruction(mainInstruction);
            }
            // 处理终止语句
            prevBlock.getTerminateInstruction().waste();
            var nextBlockTerminateInstruction = nextBlock.getTerminateInstruction();
            nextBlockTerminateInstruction.removeFromBasicBlock();
            prevBlock.setTerminateInstruction(nextBlockTerminateInstruction);
            // 处理基本块
            nextBlock.replaceAllUsageTo(prevBlock);
            nextBlock.removeFromFunction();
            changed = true;
        }
        return changed;
    }

    private static boolean runOnFunction(Function function) {
        boolean changed = false;
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            changed |= runOnBasicBlock(basicBlock);
        }
        return changed;
    }

    public static boolean runOnModule(Module module) {
        boolean changed = false;
        for (Function function : module.getFunctions()) {
            changed |= runOnFunction(function);
        }
        return changed;
    }
}
