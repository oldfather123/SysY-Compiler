package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.exception.CompilationProcessCheckFailedException;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.instruction.BranchInst;
import cn.edu.bit.newnewcc.ir.value.instruction.JumpInst;
import cn.edu.bit.newnewcc.ir.value.instruction.PhiInst;

import java.util.HashSet;
import java.util.Set;

/**
 * 跳转合并
 */
/*
 * 如果块A跳转到块B，而后立刻跳转到块C，且块C的入口不包含Phi，则改由块A直接跳转到块C
 */
public class JumpInstMergePass {

    public static boolean runOnFunction(Function function) {
        // 求解作为Phi入口的基本块
        Set<BasicBlock> blocksWithPhiExit = new HashSet<>();
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            for (Instruction leadingInstruction : basicBlock.getLeadingInstructions()) {
                if (leadingInstruction instanceof PhiInst) {
                    blocksWithPhiExit.addAll(basicBlock.getEntryBlocks());
                    break;
                }
            }
        }
        boolean changed = false;
        // 检查符合合并条件的基本块，执行合并
        for (BasicBlock eliminatedBlock : function.getBasicBlocks()) {
            if (eliminatedBlock == function.getEntryBasicBlock()) continue;
            if (blocksWithPhiExit.contains(eliminatedBlock)) continue;
            if (eliminatedBlock.getInstructions().size() > 1) continue;
            if (!(eliminatedBlock.getTerminateInstruction() instanceof JumpInst jumpInst)) continue;
            var jumpTargetBlock = jumpInst.getExit();
            for (Operand usage : eliminatedBlock.getUsages()) {
                if (usage.getInstruction() instanceof BranchInst branchInst) {
                    // IR语义上不允许 trueExit 和 falseExit 相同，此处需要额外处理
                    if (branchInst.getTrueExit() == eliminatedBlock) {
                        if (branchInst.getFalseExit() == jumpTargetBlock) {
                            var newJumpInst = new JumpInst(jumpTargetBlock);
                            branchInst.getBasicBlock().setTerminateInstruction(newJumpInst);
                            branchInst.waste();
                        } else {
                            usage.setValue(jumpTargetBlock);
                        }
                    } else if (branchInst.getFalseExit() == eliminatedBlock) {
                        if (branchInst.getTrueExit() == jumpTargetBlock) {
                            var newJumpInst = new JumpInst(jumpTargetBlock);
                            branchInst.getBasicBlock().setTerminateInstruction(newJumpInst);
                            branchInst.waste();
                        } else {
                            usage.setValue(jumpTargetBlock);
                        }
                    } else {
                        throw new CompilationProcessCheckFailedException();
                    }
                } else {
                    usage.setValue(jumpTargetBlock);
                }
            }
            assert eliminatedBlock.getUsages().isEmpty();
            changed = true;
        }
        return changed;
    }

    public static boolean runOnModule(Module module) {
        boolean changed = false;
        for (Function function : module.getFunctions()) {
            changed |= runOnFunction(function);
        }
        if (changed) {
            DeadCodeEliminationPass.runOnModule(module);
        }
        return changed;
    }
}
