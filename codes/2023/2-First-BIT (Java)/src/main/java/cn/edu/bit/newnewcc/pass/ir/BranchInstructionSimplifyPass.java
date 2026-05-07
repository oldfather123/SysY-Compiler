package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.type.IntegerType;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.constant.ConstInt;
import cn.edu.bit.newnewcc.ir.value.instruction.BranchInst;
import cn.edu.bit.newnewcc.ir.value.instruction.IntegerCompareInst;
import cn.edu.bit.newnewcc.ir.value.instruction.ZeroExtensionInst;

/**
 * 消除前端产生的冗余条件跳转语句 <br>
 */
/*
 * 前端产生的语句：
 * %1 = cmp v1, v2
 * %2 = zext i1 %1 to i32
 * %3 = icmp ne %2, 0
 * br %3, %true_block, %false_block
 *
 * 简化后的语句：
 * %1 = cmp v1, v2
 * br %1, %true_block, %false_block
 *
 */
public class BranchInstructionSimplifyPass {

    public static boolean runOnBasicBlock(BasicBlock basicBlock) {
        if (!(basicBlock.getTerminateInstruction() instanceof BranchInst branchInst)) return false;
        if (!(branchInst.getCondition() instanceof IntegerCompareInst extraCmp)) return false;
        if (!(extraCmp.getCondition() == IntegerCompareInst.Condition.NE)) return false;
        if (!(extraCmp.getOperand2() == ConstInt.getInstance(0))) return false;
        if (!(extraCmp.getOperand1() instanceof ZeroExtensionInst zeroExtensionInst)) return false;
        if (!(zeroExtensionInst.getSourceType() == IntegerType.getI1())) return false;
        branchInst.setCondition(zeroExtensionInst.getSourceOperand());
        return true;
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
        return changed;
    }
}
