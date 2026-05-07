package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.type.VoidType;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.instruction.PhiInst;
import cn.edu.bit.newnewcc.ir.value.instruction.TerminateInst;
import cn.edu.bit.newnewcc.ir.value.instruction.UnreachableInst;
import cn.edu.bit.newnewcc.pass.ir.structure.Loop;
import cn.edu.bit.newnewcc.pass.ir.structure.LoopClone;
import cn.edu.bit.newnewcc.pass.ir.structure.LoopForest;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class ConstLoopUnrollPass {

    /**
     * 单次循环展开后，展开部分语句数量总和的限制
     */
    private static final int MAXIMUM_EXTRACTED_SIZE = 5000;
    private static final int MAXIMUM_LOOP_COUNT = 128;

    private final Function function;
    private final LoopForest loopForest;

    private ConstLoopUnrollPass(Function function) {
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
        // 循环必须有固定循环次数，才能进行展开
        if (!loop.isSimpleLoop() || !loop.getSimpleLoopInfo().isFixedLoop()) return false;
        var loopBlocks = new HashSet<BasicBlock>();
        int loopSize = collectLoopInfo(loop, loopBlocks);
        // 判断展开后循环的大小，实际展开次数为循环次数+1
        var loopCount = loop.getSimpleLoopInfo().getLoopCount();
        if (loopCount > MAXIMUM_LOOP_COUNT || (long) (loopCount + 1) * loopSize > MAXIMUM_EXTRACTED_SIZE) return false;
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
        // 处理第一个复制的块
        LoopClone firstClonedLoop = new LoopClone(loop.getHeaderBasicBlock(), loopBlocks);
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
            loop.getHeaderBasicBlock().getFunction().addBasicBlock(clonedBasicBlock);
        }
        exitValueMapping.forEach((headBlockInstruction, phiInst) -> {
            phiInst.addEntry(firstClonedLoop.getClonedLoopHead(), firstClonedLoop.getClonedValue(headBlockInstruction));
        });
        // 处理剩下的复制的块
        LoopClone lastClonedLoop = firstClonedLoop;
        for (int i = 0; i < loopCount; i++) {
            LoopClone clonedLoop = new LoopClone(loop.getHeaderBasicBlock(), loopBlocks);
            lastClonedLoop.setNextLoopHead(clonedLoop.getClonedLoopHead());
            LoopClone finalLastClonedLoop = lastClonedLoop;
            innerEntries.forEach((basicBlock, phiInstValueMap) -> {
                var newMap = new HashMap<PhiInst, Value>();
                phiInstValueMap.forEach((phiInst, value) -> newMap.put(phiInst, finalLastClonedLoop.getClonedValue(value)));
                clonedLoop.addEntry((BasicBlock) finalLastClonedLoop.getClonedValue(basicBlock), newMap);
            });
            for (BasicBlock clonedBasicBlock : clonedLoop.getClonedBasicBlocks()) {
                loop.getHeaderBasicBlock().getFunction().addBasicBlock(clonedBasicBlock);
            }
            exitValueMapping.forEach((headBlockInstruction, phiInst) -> {
                phiInst.addEntry(clonedLoop.getClonedLoopHead(), clonedLoop.getClonedValue(headBlockInstruction));
            });
            lastClonedLoop = clonedLoop;
        }
        // 将最后一个块连接到黑洞
        var blackHole = new BasicBlock();
        function.addBasicBlock(blackHole);
        blackHole.setTerminateInstruction(new UnreachableInst());
        lastClonedLoop.setNextLoopHead(blackHole);
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

    private boolean runOnFunction() {
        boolean changed = false;
        for (Loop rootLoop : loopForest.getRootLoops()) {
            changed |= tryUnrollLoop(rootLoop);
        }
        return changed;
    }

    public static boolean runOnFunction(Function function) {
        return new ConstLoopUnrollPass(function).runOnFunction();
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
