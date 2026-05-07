package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.exception.IllegalStateException;
import cn.edu.bit.newnewcc.ir.type.IntegerType;
import cn.edu.bit.newnewcc.ir.type.VoidType;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.constant.ConstBool;
import cn.edu.bit.newnewcc.ir.value.constant.ConstLong;
import cn.edu.bit.newnewcc.ir.value.instruction.*;
import cn.edu.bit.newnewcc.pass.ir.structure.Loop;
import cn.edu.bit.newnewcc.pass.ir.structure.LoopClone;
import cn.edu.bit.newnewcc.pass.ir.structure.LoopForest;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class LoopUnrollPass {
    /**
     * 展开前循环大小的限制
     */
    private static final int MAXIMUM_EXTRACTED_SIZE = 10;

    /**
     * 循环展开的路数，通常选择四路展开
     */
    private static final int UNROLLING_FACTOR = 4;

    private final Function function;
    private final LoopForest loopForest;

    private LoopUnrollPass(Function function) {
        this.function = function;
        this.loopForest = LoopForest.buildOver(function);
    }

    private int collectLoopInfo(Loop loop, Set<BasicBlock> visitedBlocks) {
        int loopSize = 0;
        for (BasicBlock basicBlock : loop.getBasicBlocks()) {
            visitedBlocks.add(basicBlock);
            loopSize += basicBlock.getInstructions().size();
        }
        for (Loop subLoop : loop.getSubLoops()) {
            loopSize += collectLoopInfo(subLoop, visitedBlocks);
        }
        return loopSize;
    }

    private boolean tryUnrollLoop(Loop loop) {
        for (Loop subLoop : loop.getSubLoops()) {
            // 单次只能展开一个循环，展开后需要重新进行循环分析
            if (tryUnrollLoop(subLoop)) return true;
        }
        // 只展开简单循环
        if (!loop.isSimpleLoop()) return false;
        var loopConditionType = loop.getSimpleLoopInfo().condition().getCondition();
        if (loopConditionType == IntegerCompareInst.Condition.EQ || loopConditionType == IntegerCompareInst.Condition.NE)
            return false;
        var loopBlocks = new HashSet<BasicBlock>();
        int loopSize = collectLoopInfo(loop, loopBlocks);
        // 待展开的循环大小不超过 MAXIMUM_EXTRACTED_SIZE
        if (loopSize > MAXIMUM_EXTRACTED_SIZE) return false;
        if (loop.getHeaderBasicBlock().tags.contains(BasicBlock.Tag.NO_LOOP_UNROLL)) return false;
        var function = loop.getHeaderBasicBlock().getFunction();
        // 复制一份原始循环，稍后将其连接到自身
        // 如此操作是为了避免修改原循环，导致一边复制一边修改
        // 所有操作后，原循环将被废弃
        LoopClone remainderLoop = new LoopClone(loop.getHeaderBasicBlock(), loopBlocks);
        // 将headBlock中的值引入到出口块中
        var loopExitBlock = loop.getSimpleLoopInfo().exitBlock();
        var exitValueMapping = new HashMap<Instruction, PhiInst>();
        for (Instruction instruction : loop.getHeaderBasicBlock().getInstructions()) {
            if (instruction.getType() != VoidType.getInstance()) {
                PhiInst exitPhi = null;
                for (Operand usage : instruction.getUsages()) {
                    if (usage.getInstruction() instanceof PhiInst phiInst && phiInst.getBasicBlock() == loopExitBlock) {
                        exitPhi = phiInst;
                        break;
                    }
                }
                if (exitPhi == null) {
                    exitPhi = new PhiInst(instruction.getType());
                    loopExitBlock.addInstruction(exitPhi);
                }
                exitValueMapping.put(instruction, exitPhi);
                for (Operand usage : instruction.getUsages()) {
                    if (!loopBlocks.contains(usage.getInstruction().getBasicBlock())) {
                        usage.setValue(exitPhi);
                    }
                }
            }
        }
        // 构建展开后的循环
        LoopClone firstClonedLoop = new LoopClone(loop.getHeaderBasicBlock(), loopBlocks);
        makeExitConditionExtracted(loop, firstClonedLoop);
        var firstLoopExitInstruction = ((BranchInst) firstClonedLoop.getClonedLoopHead().getTerminateInstruction());
        firstLoopExitInstruction.setFalseExit(remainderLoop.getClonedLoopHead());
        Map<BasicBlock, Map<PhiInst, Value>> outerEntries = new HashMap<>();
        Map<BasicBlock, Map<PhiInst, Value>> innerEntries = new HashMap<>();
        for (Operand usage : loop.getHeaderBasicBlock().getUsages()) {
            if (usage.getInstruction() instanceof TerminateInst) {
                var entryBlock = usage.getInstruction().getBasicBlock();
                if (loopBlocks.contains(entryBlock)) {
                    innerEntries.put(entryBlock, new HashMap<>());
                } else {
                    usage.setValue(firstClonedLoop.getClonedLoopHead());
                    outerEntries.put(entryBlock, new HashMap<>());
                }
            }
        }
        for (Instruction leadingInstruction : loop.getHeaderBasicBlock().getLeadingInstructions()) {
            if (leadingInstruction instanceof PhiInst phiInst) {
                phiInst.forEach((basicBlock, value) -> {
                    if (outerEntries.containsKey(basicBlock)) {
                        outerEntries.get(basicBlock).put(phiInst, value);
                    } else {
                        innerEntries.get(basicBlock).put(phiInst, value);
                    }
                });
            }
        }
        outerEntries.forEach(firstClonedLoop::addEntry);
        for (BasicBlock clonedBasicBlock : firstClonedLoop.getClonedBasicBlocks()) {
            function.addBasicBlock(clonedBasicBlock);
        }
        concatFalseExit(loop, firstClonedLoop, remainderLoop);
        // 处理剩下的复制的循环
        LoopClone lastClonedLoop = firstClonedLoop;
        for (int i = 1; i < UNROLLING_FACTOR; i++) {
            LoopClone clonedLoop = new LoopClone(loop.getHeaderBasicBlock(), loopBlocks);
            ((BranchInst) clonedLoop.getClonedLoopHead().getTerminateInstruction()).setCondition(ConstBool.getInstance(true));
            ((BranchInst) clonedLoop.getClonedLoopHead().getTerminateInstruction()).setFalseExit(remainderLoop.getClonedLoopHead());
            concatLoop(innerEntries, clonedLoop, lastClonedLoop);
            for (BasicBlock clonedBasicBlock : clonedLoop.getClonedBasicBlocks()) {
                function.addBasicBlock(clonedBasicBlock);
            }
            concatFalseExit(loop, clonedLoop, remainderLoop);
            lastClonedLoop = clonedLoop;
        }
        // 将最后一次循环连接到开头
        concatLoop(innerEntries, firstClonedLoop, lastClonedLoop);
        // 配置 remainder loop
        concatLoop(innerEntries, remainderLoop, remainderLoop);
        for (BasicBlock clonedBasicBlock : remainderLoop.getClonedBasicBlocks()) {
            function.addBasicBlock(clonedBasicBlock);
        }
        exitValueMapping.forEach((headBlockInstruction, phiInst) -> {
            phiInst.addEntry(remainderLoop.getClonedLoopHead(), remainderLoop.getClonedValue(headBlockInstruction));
        });
        remainderLoop.getClonedLoopHead().tags.add(BasicBlock.Tag.NO_LOOP_UNROLL);
        // 删除原循环
        // 1. 清理出口信息
        for (BasicBlock loopBlock : loopBlocks) {
            for (BasicBlock exitBlock : loopBlock.getExitBlocks()) {
                exitBlock.removeEntryFromPhi(loopBlock);
            }
            var tempTerminate = loopBlock.getTerminateInstruction();
            loopBlock.setTerminateInstruction(new UnreachableInst());
            tempTerminate.waste();
        }
        // 2. 清理操作数
        for (BasicBlock loopBlock : loopBlocks) {
            for (Instruction instruction : loopBlock.getInstructions()) {
                instruction.clearOperands();
            }
        }
        // 3. 移除无效块
        for (BasicBlock loopBlock : loopBlocks) {
            for (Instruction instruction : loopBlock.getInstructions()) {
                instruction.waste();
            }
            loopBlock.removeFromFunction();
        }
        return true;
    }

    private static void concatLoop(Map<BasicBlock, Map<PhiInst, Value>> innerEntries, LoopClone secondLoop, LoopClone firstLoop) {
        firstLoop.setNextLoopHead(secondLoop.getClonedLoopHead());
        innerEntries.forEach((basicBlock, phiInstValueMap) -> {
            var newMap = new HashMap<PhiInst, Value>();
            phiInstValueMap.forEach((phiInst, value) -> newMap.put(phiInst, firstLoop.getClonedValue(value)));
            secondLoop.addEntry((BasicBlock) firstLoop.getClonedValue(basicBlock), newMap);
        });
    }

    private static void concatFalseExit(Loop originalLoop, LoopClone firstLoop, LoopClone secondLoop) {
        var newMap = new HashMap<PhiInst, Value>();
        for (Instruction leadingInstruction : originalLoop.getHeaderBasicBlock().getLeadingInstructions()) {
            if (leadingInstruction instanceof PhiInst phiInst) {
                newMap.put(phiInst, firstLoop.getClonedValue(phiInst));
            }
        }
        secondLoop.addEntry(firstLoop.getClonedLoopHead(), newMap);
    }

    private static void makeExitConditionExtracted(Loop loop, LoopClone firstClonedLoop) {
        // 修改退出条件为连续四次循环后的条件
        var stepValue32 = firstClonedLoop.getClonedValue(loop.getSimpleLoopInfo().stepInst().getOperand2()); // SimpleLoopInfo 构建时保证静态值位于操作数2
        var stepValue64 = new SignedExtensionInst(stepValue32, IntegerType.getI64());
        var beforeStepValue32 = firstClonedLoop.getClonedValue(loop.getSimpleLoopInfo().stepInst().getOperand1());
        var beforeStepValue64 = new SignedExtensionInst(beforeStepValue32, IntegerType.getI64());
        var newStepValue64 = new IntegerMultiplyInst(IntegerType.getI64(), stepValue64, ConstLong.getInstance(UNROLLING_FACTOR));
        IntegerArithmeticInst afterStepValue64;
        if (loop.getSimpleLoopInfo().stepInst() instanceof IntegerAddInst) {
            afterStepValue64 = new IntegerAddInst(IntegerType.getI64(), beforeStepValue64, newStepValue64);
        } else if (loop.getSimpleLoopInfo().stepInst() instanceof IntegerSubInst) {
            afterStepValue64 = new IntegerSubInst(IntegerType.getI64(), beforeStepValue64, newStepValue64);
        } else {
            throw new IllegalStateException("Unknown step instruction.");
        }
        var limitValue32 = firstClonedLoop.getClonedValue(loop.getSimpleLoopInfo().condition().getOperand2());
        var limitValue64 = new SignedExtensionInst(limitValue32, IntegerType.getI64());
        var newCompareInst = new IntegerCompareInst(
                IntegerType.getI64(),
                loop.getSimpleLoopInfo().condition().getCondition(),
                afterStepValue64,
                limitValue64
        );
        var firstLoopEntry = firstClonedLoop.getClonedLoopHead();
        firstLoopEntry.addInstruction(stepValue64);
        firstLoopEntry.addInstruction(beforeStepValue64);
        firstLoopEntry.addInstruction(newStepValue64);
        firstLoopEntry.addInstruction(afterStepValue64);
        firstLoopEntry.addInstruction(limitValue64);
        firstLoopEntry.addInstruction(newCompareInst);
        ((BranchInst) firstLoopEntry.getTerminateInstruction()).setCondition(newCompareInst);
    }

    private boolean runOnFunction() {
        boolean changed = false;
        for (Loop rootLoop : loopForest.getRootLoops()) {
            changed |= tryUnrollLoop(rootLoop);
        }
        return changed;
    }

    public static boolean runOnFunction(Function function) {
        return new LoopUnrollPass(function).runOnFunction();
    }

    public static boolean runOnModule(Module module) {
        boolean changed = false;
        for (Function function : module.getFunctions()) {
            boolean result;
            do {
                result = runOnFunction(function);
                changed |= result;
            } while (result);
        }
        if (changed) {
            DeadCodeEliminationPass.runOnModule(module);
        }
        return changed;
    }

}
