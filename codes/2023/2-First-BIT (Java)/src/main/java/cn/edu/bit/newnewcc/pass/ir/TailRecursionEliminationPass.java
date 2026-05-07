package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.type.VoidType;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.instruction.CallInst;
import cn.edu.bit.newnewcc.ir.value.instruction.JumpInst;
import cn.edu.bit.newnewcc.ir.value.instruction.PhiInst;
import cn.edu.bit.newnewcc.ir.value.instruction.ReturnInst;

import java.util.HashMap;
import java.util.Map;

public class TailRecursionEliminationPass {

    private static boolean isTailRecursionBlock(Function function, BasicBlock basicBlock) {
        // 终止指令是返回
        if (!(basicBlock.getTerminateInstruction() instanceof ReturnInst returnInst)) return false;
        // 最后一条指令是对自身的call
        var mainInstructions = basicBlock.getMainInstructions();
        if (mainInstructions.size() == 0) return false;
        var lastInstruction = mainInstructions.get(mainInstructions.size() - 1);
        if (!(lastInstruction instanceof CallInst callInst && callInst.getCallee() == function)) return false;
        // 返回值是call的结果（或void）
        if (function.getReturnType() != VoidType.getInstance()) {
            if (returnInst.getReturnValue() != callInst) return false;
        }
        return true;
    }

    private static boolean hasTailRecursion(Function function) {
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            if (isTailRecursionBlock(function, basicBlock)) return true;
        }
        return false;
    }

    private static boolean runOnFunction(Function function) {
        if (!hasTailRecursion(function)) return false;
        // 构造 phiBlock ，用于实现形参的多路选择
        // 将 entryBlock 里的指令移动到 phiBlock 中，使得 entryBlock 里只留下 alloca
        var functionEntryBlock = function.getEntryBasicBlock();
        BasicBlock phiBlock;
        if (!(functionEntryBlock.getMainInstructions().size() == 0 && functionEntryBlock.getTerminateInstruction() instanceof JumpInst jumpInst)) {
            phiBlock = new BasicBlock();
            function.addBasicBlock(phiBlock);
            for (Instruction instruction : functionEntryBlock.getMainInstructions()) {
                instruction.removeFromBasicBlock();
                phiBlock.addInstruction(instruction);
            }
            var entryTerminateInst = functionEntryBlock.getTerminateInstruction();
            entryTerminateInst.removeFromBasicBlock();
            functionEntryBlock.setTerminateInstruction(new JumpInst(phiBlock));
            phiBlock.setTerminateInstruction(entryTerminateInst);
            functionEntryBlock.replaceAllUsageTo(phiBlock);
        } else {
            phiBlock = jumpInst.getExit();
        }
        // 创建所有形参的phi指令
        Map<Function.FormalParameter, PhiInst> phiInstMap = new HashMap<>();
        // 收集已有的形参phi指令
        for (Instruction leadingInstruction : phiBlock.getLeadingInstructions()) {
            if (leadingInstruction instanceof PhiInst phiInst) {
                if (phiInst.getValue(functionEntryBlock) instanceof Function.FormalParameter formalParameter) {
                    phiInstMap.put(formalParameter, phiInst);
                }
            }
        }
        // 创建不存在的形参phi指令
        for (Function.FormalParameter formalParameter : function.getFormalParameters()) {
            if (!phiInstMap.containsKey(formalParameter)) {
                var phiInst = new PhiInst(formalParameter.getType());
                // 原有情况下，所有指令都是直接使用 formalParameter 的
                // 将这些使用替换为对新建phi指令的使用
                formalParameter.replaceAllUsageTo(phiInst);
                // 设置phi的入口值
                for (BasicBlock phiEntry : phiBlock.getEntryBlocks()) {
                    if (phiEntry == functionEntryBlock) {
                        phiInst.addEntry(phiEntry, formalParameter);
                    } else {
                        phiInst.addEntry(phiEntry, phiInst);
                    }
                }
                // 将 phiInst 加入基本块
                phiBlock.addInstruction(phiInst);
                // 将 phiInst 加入形参到phi的映射
                phiInstMap.put(formalParameter, phiInst);
            }
        }
        // 执行尾递归消除
        for (BasicBlock block : function.getBasicBlocks()) {
            if (!isTailRecursionBlock(function, block)) continue;
            var instructions = block.getMainInstructions();
            var callInst = (CallInst) instructions.get(instructions.size() - 1);
            int argumentSize = function.getFormalParameters().size();
            // 将call指令移除
            callInst.removeFromBasicBlock();
            // 替换终止指令
            block.getTerminateInstruction().waste();
            block.setTerminateInstruction(new JumpInst(phiBlock));
            // 正确设置phi指令
            for (int i = 0; i < argumentSize; i++) {
                var formalParameter = function.getFormalParameters().get(i);
                var argument = callInst.getArgumentAt(i);
                var phiInst = phiInstMap.get(formalParameter);
                phiInst.addEntry(block, argument);
            }
            // 删除call指令
            callInst.waste();
        }
        return true;
    }

    public static boolean runOnModule(Module module) {
        boolean changed = false;
        for (Function function : module.getFunctions()) {
            changed |= runOnFunction(function);
        }
        return changed;
    }

}
