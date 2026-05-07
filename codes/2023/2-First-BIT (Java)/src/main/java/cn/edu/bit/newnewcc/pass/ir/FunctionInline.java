package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.type.VoidType;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.instruction.AllocateInst;
import cn.edu.bit.newnewcc.ir.value.instruction.CallInst;
import cn.edu.bit.newnewcc.ir.value.instruction.JumpInst;
import cn.edu.bit.newnewcc.ir.value.instruction.PhiInst;
import cn.edu.bit.newnewcc.pass.ir.structure.FunctionClone;

import java.util.*;

public class FunctionInline {

    private static final int INLINE_SIZE_THRESHOLD = 100;

    private static class FunctionProperty {
        public int size;
        public final Set<Function> callees;

        public FunctionProperty(int size, Set<Function> callees) {
            this.size = size;
            this.callees = callees;
        }

    }

    private final Module module;
    private final Map<Function, FunctionProperty> propertyMap = new HashMap<>();

    private FunctionInline(Module module) {
        this.module = module;
    }

    /**
     * 收集相关信息
     */
    private void collectInformation() {
        for (Function function : module.getFunctions()) {
            Set<Function> callees = new HashSet<>();
            int size = 0;
            for (BasicBlock basicBlock : function.getBasicBlocks()) {
                for (Instruction mainInstruction : basicBlock.getMainInstructions()) {
                    if (mainInstruction instanceof CallInst callInst) {
                        if (callInst.getCallee() instanceof Function callee) {
                            callees.add(callee);
                        }
                    }
                    size++;
                }
            }
            propertyMap.put(function, new FunctionProperty(size, callees));
        }
    }

    private void inlineFunction(Function inlinedFunction) {
        for (Operand usage : inlinedFunction.getUsages()) {
            if (!(usage.getInstruction() instanceof CallInst callInst && callInst.getCallee() == inlinedFunction))
                continue;
            var callerFunction = callInst.getBasicBlock().getFunction();
            // 将原有基本块按照 call 指令的位置拆分为两个
            var blockAlpha = callInst.getBasicBlock();
            var blockBeta = new BasicBlock();
            boolean startMoving = false;
            for (Instruction instruction : blockAlpha.getMainInstructions()) {
                if (startMoving) {
                    instruction.removeFromBasicBlock();
                    blockBeta.addInstruction(instruction);
                }
                if (instruction == callInst) startMoving = true;
            }
            var betaTerminate = blockAlpha.getTerminateInstruction();
            betaTerminate.removeFromBasicBlock();
            blockBeta.setTerminateInstruction(betaTerminate);
            for (Operand blockAlphaUsage : blockAlpha.getUsages()) {
                if (blockAlphaUsage.getInstruction() instanceof PhiInst) {
                    blockAlphaUsage.setValue(blockBeta);
                }
            }
            // 插入内联后的函数
            var clonedFunction = new FunctionClone(inlinedFunction, callInst.getArgumentList(), blockBeta);
            blockAlpha.setTerminateInstruction(new JumpInst(clonedFunction.getEntryBlock()));
            callerFunction.addBasicBlock(blockBeta);
            for (BasicBlock clonedBlock : clonedFunction.getBasicBlocks()) {
                callerFunction.addBasicBlock(clonedBlock);
            }
            if (callInst.getType() != VoidType.getInstance()) {
                blockBeta.addInstruction(clonedFunction.getReturnValue());
                callInst.replaceAllUsageTo(clonedFunction.getReturnValue());
            } else {
                clonedFunction.getReturnValue().waste();
            }
            callInst.waste();
            for (AllocateInst allocateInstruction : clonedFunction.getAllocateInstructions()) {
                callerFunction.getEntryBasicBlock().addInstruction(allocateInstruction);
            }
            propertyMap.get(callerFunction).size += propertyMap.get(inlinedFunction).size;
        }
    }

    private boolean runOnModule() {
        boolean changed = false;
        collectInformation();
        List<Function> functions = new ArrayList<>(module.getFunctions());
        functions.sort((function1, function2) -> Integer.compare(propertyMap.get(function1).size, propertyMap.get(function2).size));
        for (Function inlinedFunction : functions) {
            if (inlinedFunction.getValueName().equals("main")) continue;
            if (propertyMap.get(inlinedFunction).callees.contains(inlinedFunction)) continue;
            if (propertyMap.get(inlinedFunction).size > INLINE_SIZE_THRESHOLD) continue;
            inlineFunction(inlinedFunction);
            changed = true;
        }
        if (changed) {
            DeadCodeEliminationPass.runOnModule(module);
        }
        return changed;
    }

    public static boolean runOnModule(Module module) {
        return new FunctionInline(module).runOnModule();
    }

}
