package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.instruction.*;

import java.util.HashSet;
import java.util.Set;

public class BranchToMinMaxPass {

    private static boolean runOnBasicBlock(BasicBlock branchBlock) {
        // 以分支块开始
        if (!(branchBlock.getTerminateInstruction() instanceof BranchInst br)) return false;
        // 比较条件是整数比较，且符号不是等号
        if (!(br.getCondition() instanceof IntegerCompareInst icmp)) return false;
        if (icmp.getCondition() == IntegerCompareInst.Condition.EQ ||
                icmp.getCondition() == IntegerCompareInst.Condition.NE) return false;
        var v1 = icmp.getOperand1();
        var v2 = icmp.getOperand2();
        var trueBlock = br.getTrueExit();
        var falseBlock = br.getFalseExit();
        // 两个分支出口都只有分支块这一个入口
        if (trueBlock.getEntryBlocks().size() > 1 || falseBlock.getEntryBlocks().size() > 1) return false;
        // 两个分支出口块都是空的，且都跳到了同一个目标块
        if (trueBlock.getInstructions().size() > 1 || falseBlock.getInstructions().size() > 1) return false;
        if (trueBlock.getTerminateInstruction() instanceof JumpInst trueJump &&
                falseBlock.getTerminateInstruction() instanceof JumpInst falseJump) {
            if (trueJump.getExit() != falseJump.getExit()) return false;
            var endingBlock = trueJump.getExit();
            // 收尾块的入口只有两个分支出口
            if (endingBlock.getEntryBlocks().size() > 2) return false;
            // 收尾块只有关于v1和v2的phi
            Set<PhiInst> minInstructions = new HashSet<>();
            Set<PhiInst> maxInstructions = new HashSet<>();
            for (Instruction leadingInstruction : endingBlock.getLeadingInstructions()) {
                if (leadingInstruction instanceof PhiInst phiInst) {
                    var valueTrue = phiInst.getValue(trueBlock);
                    var valueFalse = phiInst.getValue(falseBlock);
                    if (valueTrue != v1 && valueTrue != v2) return false;
                    if (valueFalse != v1 && valueFalse != v2) return false;
                    if (valueTrue == valueFalse) return false; // 由 ConstantFolding 处理
                    switch (icmp.getCondition()) {
                        case SGE, SGT -> {
                            if (valueTrue == v1) {
                                maxInstructions.add(phiInst);
                            } else {
                                minInstructions.add(phiInst);
                            }
                        }
                        case SLE, SLT -> {
                            if (valueTrue == v1) {
                                minInstructions.add(phiInst);
                            } else {
                                maxInstructions.add(phiInst);
                            }
                        }
                    }
                }
            }
            boolean found = false;
            // 将 phi 指令替换为 min/max 指令
            if (!minInstructions.isEmpty()) {
                found = true;
                var minInst = new SignedMinInst(icmp.getComparedType(), v1, v2);
                endingBlock.addMainInstructionAtBeginning(minInst);
                for (PhiInst phiInst : minInstructions) {
                    phiInst.replaceAllUsageTo(minInst);
                    phiInst.waste();
                }
            }
            if (!maxInstructions.isEmpty()) {
                found = true;
                var maxInst = new SignedMaxInst(icmp.getComparedType(), v1, v2);
                endingBlock.addMainInstructionAtBeginning(maxInst);
                for (PhiInst phiInst : maxInstructions) {
                    phiInst.replaceAllUsageTo(maxInst);
                    phiInst.waste();
                }
            }
            return found;
        } else if (trueBlock.getTerminateInstruction() instanceof ReturnInst trueReturn &&
                falseBlock.getTerminateInstruction() instanceof ReturnInst falseReturn) {
            var valueTrue = trueReturn.getReturnValue();
            var valueFalse = falseReturn.getReturnValue();
            ReturnInst replacedReturnInst;
            if (valueTrue == valueFalse) {
                replacedReturnInst = new ReturnInst(valueTrue);
            } else {
                if (valueTrue != v1 && valueTrue != v2) return false;
                if (valueFalse != v1 && valueFalse != v2) return false;
                var mInst = switch (icmp.getCondition()) {
                    case SGE, SGT -> {
                        if (valueTrue == v1) {
                            yield new SignedMaxInst(icmp.getComparedType(), v1, v2);
                        } else {
                            yield new SignedMinInst(icmp.getComparedType(), v1, v2);
                        }
                    }
                    case SLE, SLT -> {
                        if (valueTrue == v1) {
                            yield new SignedMinInst(icmp.getComparedType(), v1, v2);
                        } else {
                            yield new SignedMaxInst(icmp.getComparedType(), v1, v2);
                        }
                    }
                    default -> {
                        // 此前应当已经检查过，比较类型不能是等于和不等于
                        throw new RuntimeException();
                    }
                };
                branchBlock.addInstruction(mInst);
                replacedReturnInst = new ReturnInst(mInst);
            }
            branchBlock.setTerminateInstruction(replacedReturnInst);
            br.waste();
            return true;
        } else {
            return false;
        }
    }

    public static boolean runOnFunction(Function function) {
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
        if (changed) {
            DeadCodeEliminationPass.runOnModule(module);
            JumpInstMergePass.runOnModule(module);
        }
        return changed;
    }
}
