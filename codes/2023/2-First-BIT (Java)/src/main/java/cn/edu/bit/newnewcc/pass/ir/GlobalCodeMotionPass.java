package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.exception.CompilationProcessCheckFailedException;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.instruction.CallInst;
import cn.edu.bit.newnewcc.ir.value.instruction.MemoryInst;
import cn.edu.bit.newnewcc.ir.value.instruction.PhiInst;
import cn.edu.bit.newnewcc.ir.value.instruction.TerminateInst;
import cn.edu.bit.newnewcc.pass.ir.structure.DomTree;
import cn.edu.bit.newnewcc.pass.ir.structure.LoopForest;

import java.util.*;
import java.util.function.BiConsumer;
import java.util.function.BiFunction;
import java.util.function.Consumer;

/**
 * GCM（全局代码提升） <br>
 * 需要保证运行的Module已经通过了DeadCodeElimination <br>
 */
public class GlobalCodeMotionPass {

    private final DomTree domTree;
    private final Map<BasicBlock, Integer> domDepthMap;
    private final Map<BasicBlock, Integer> loopDepthMap;
    private final Set<BasicBlock> functionBasicBlocks;
    private final Map<Instruction, Set<Instruction>> predecessors = new HashMap<>(), successors = new HashMap<>();
    private final Map<Instruction, BasicBlock> earlyPlacement = new HashMap<>(), latePlacement = new HashMap<>();

    private GlobalCodeMotionPass(DomTree domTree, Map<BasicBlock, Integer> domDepthMap, Map<BasicBlock, Integer> loopDepthMap) {
        this.domTree = domTree;
        this.domDepthMap = domDepthMap;
        this.loopDepthMap = loopDepthMap;
        this.functionBasicBlocks = loopDepthMap.keySet();
    }

    private void collectPredecessorsAndSuccessors() {
        for (BasicBlock basicBlock : functionBasicBlocks) {
            for (Instruction instruction : basicBlock.getInstructions()) {
                predecessors.put(instruction, new HashSet<>());
                successors.put(instruction, new HashSet<>());
            }
        }
        for (BasicBlock basicBlock : functionBasicBlocks) {
            for (Instruction instruction : basicBlock.getInstructions()) {
                if (instruction instanceof PhiInst phiInst) {
                    // Phi指令其实是分散在各个entry末尾的多个指令的集合
                    // 它们的本体在IR中不存在，因此借用entry的terminateInst来限制
                    phiInst.forEach((entry, value) -> {
                        if (value instanceof Instruction predecessor) {
                            predecessors.get(entry.getTerminateInstruction()).add(predecessor);
                            successors.get(predecessor).add(entry.getTerminateInstruction());
                        }
                    });
                } else {
                    for (Operand operand : instruction.getOperandList()) {
                        if (operand.getValue() instanceof Instruction predecessor) {
                            predecessors.get(instruction).add(predecessor);
                            successors.get(predecessor).add(instruction);
                        }
                    }
                }
            }
        }
        for (BasicBlock basicBlock : functionBasicBlocks) {
            for (Instruction instruction : basicBlock.getInstructions()) {
                predecessors.put(instruction, Collections.unmodifiableSet(predecessors.get(instruction)));
                successors.put(instruction, Collections.unmodifiableSet(successors.get(instruction)));
            }
        }
    }

    /**
     * 判断指令位置是否被固定 <br>
     * 被固定的指令包括：Phi、内存指令、终止指令 <br>
     *
     * @param instruction 指令
     * @return 指令被固定返回true；否则返回false。
     */
    private boolean isFixedInstruction(Instruction instruction) {
        return instruction instanceof PhiInst || instruction instanceof MemoryInst ||
                instruction instanceof TerminateInst || instruction instanceof CallInst;
    }

    private void scheduleEarly() {
        Stack<Instruction> readyInstructions = new Stack<>();
        // 拓扑排序时会修改predecessors，但scheduleLate会再用到，所以需要复制一遍
        var predecessors = new HashMap<Instruction, Set<Instruction>>();
        this.predecessors.forEach((instruction, list) -> predecessors.put(instruction, new HashSet<>(list)));
        for (BasicBlock basicBlock : functionBasicBlocks) {
            for (Instruction instruction : basicBlock.getInstructions()) {
                if (!isFixedInstruction(instruction) && predecessors.get(instruction).size() == 0) {
                    readyInstructions.add(instruction);
                }
            }
        }
        // 指令放置完毕，更新localPredecessors
        Consumer<Instruction> onInstructionPlaced = placedInstruction -> {
            for (Instruction successor : successors.get(placedInstruction)) {
                if (!isFixedInstruction(successor)) {
                    var updatedPredecessors = predecessors.get(successor);
                    updatedPredecessors.remove(placedInstruction);
                    if (updatedPredecessors.size() == 0) {
                        readyInstructions.add(successor);
                    }
                }
            }
        };
        // 递归搜索支配树，尝试尽早放置指令
        var searcher = new Consumer<BasicBlock>() {
            @Override
            public void accept(BasicBlock basicBlock) {
                for (Instruction instruction : basicBlock.getInstructions()) {
                    // 非leading指令前可以安插其他指令
                    if (!BasicBlock.isLeadingInstruction(instruction)) {
                        while (!readyInstructions.isEmpty()) {
                            var puttingInstruction = readyInstructions.pop();
                            earlyPlacement.put(puttingInstruction, basicBlock);
                            onInstructionPlaced.accept(puttingInstruction);
                        }
                    }
                    // 如果是fixed指令，则立即安放该指令
                    if (isFixedInstruction(instruction)) {
                        onInstructionPlaced.accept(instruction);
                    }
                }
                for (BasicBlock domSon : domTree.getDomSons(basicBlock)) {
                    accept(domSon);
                }
            }
        };
        searcher.accept(domTree.getDomRoot());
    }

    private void scheduleLate() {
        Stack<Instruction> readyInstructions = new Stack<>();
        var successors = new HashMap<Instruction, Set<Instruction>>();
        this.successors.forEach((instruction, list) -> successors.put(instruction, new HashSet<>(list)));
        for (BasicBlock basicBlock : functionBasicBlocks) {
            for (Instruction instruction : basicBlock.getInstructions()) {
                if (!isFixedInstruction(instruction) && successors.get(instruction).size() == 0) {
                    // 没用的指令，理论上应该已经被DCE清理了
                    throw new CompilationProcessCheckFailedException("Unused code found.");
                }
            }
        }
        // 指令放置完毕，更新localSuccessors
        BiConsumer<Instruction, BasicBlock> onInstructionPlaced = (placedInstruction, basicBlock) -> {
            for (Instruction predecessor : predecessors.get(placedInstruction)) {
                if (!isFixedInstruction(predecessor)) {
                    var updatedSuccessors = successors.get(predecessor);
                    updatedSuccessors.remove(placedInstruction);
                    if (!latePlacement.containsKey(predecessor)) {
                        latePlacement.put(predecessor, basicBlock);
                    } else {
                        latePlacement.put(predecessor, domTree.lca(basicBlock, latePlacement.get(predecessor)));
                    }
                    if (updatedSuccessors.size() == 0) {
                        readyInstructions.add(predecessor);
                    }
                }
            }
        };
        // 递归搜索支配树，尝试尽晚放置指令
        var searcher = new Consumer<BasicBlock>() {
            @Override
            public void accept(BasicBlock basicBlock) {
                for (BasicBlock domSon : domTree.getDomSons(basicBlock)) {
                    accept(domSon);
                }
                // 倒序遍历所有指令
                var instructionList = basicBlock.getInstructions();
                ListIterator<Instruction> iterator = instructionList.listIterator(instructionList.size());
                while (iterator.hasPrevious()) {
                    var instruction = iterator.previous();
                    // 如果是fixed指令，则立即安放该指令
                    if (isFixedInstruction(instruction)) {
                        onInstructionPlaced.accept(instruction, basicBlock);
                        while (!readyInstructions.isEmpty()) {
                            var puttingInstruction = readyInstructions.pop();
                            onInstructionPlaced.accept(puttingInstruction, latePlacement.get(puttingInstruction));
                        }
                    }
                }
            }
        };
        searcher.accept(domTree.getDomRoot());
    }

    // 重新放置指令（不是替换）
    private void replaceInstructions() {
        // 求解 shallowestBlocks
        Map<BasicBlock, List<BasicBlock>> shallowestBlocks = new HashMap<>();
        var searcher = new BiConsumer<BasicBlock, BasicBlock>() {
            @Override
            public void accept(BasicBlock basicBlock, BasicBlock parent) {
                var bexpList = new ArrayList<BasicBlock>();
                if (parent != null) bexpList.add(parent);
                for (int i = 1; domTree.getBexpFather(basicBlock, i) != null; i++) {
                    var b1 = shallowestBlocks.get(domTree.getBexpFather(basicBlock, i - 1)).get(i - 1);
                    var b2 = bexpList.get(i - 1);
                    if (loopDepthMap.get(b1) < loopDepthMap.get(b2)) {
                        bexpList.add(b1);
                    } else {
                        bexpList.add(b2);
                    }
                }
                shallowestBlocks.put(basicBlock, bexpList);
                for (BasicBlock domSon : domTree.getDomSons(basicBlock)) {
                    accept(domSon, basicBlock);
                }
            }
        };
        searcher.accept(domTree.getDomRoot(), null);
        // 求解某个区间内循环深度最浅的基本块
        BiFunction<BasicBlock, BasicBlock, BasicBlock> getShallowestBlock = (earlyPlacement, latePlacement) -> {
            var target = latePlacement;
            for (int i = 0, dd = domDepthMap.get(latePlacement) - domDepthMap.get(earlyPlacement); (1 << i) <= dd; i++) {
                if ((dd & (1 << i)) != 0) {
                    var pTarget = shallowestBlocks.get(latePlacement).get(i);
                    if (loopDepthMap.get(pTarget) < loopDepthMap.get(target)) {
                        target = pTarget;
                    }
                    latePlacement = domTree.getBexpFather(latePlacement, i);
                }
            }
            return target;
        };
        // 求解每个基本块需要放置哪些指令
        Map<BasicBlock, List<Instruction>> pendingInstructionsMap = new HashMap<>();
        for (BasicBlock block : functionBasicBlocks) {
            pendingInstructionsMap.put(block, new ArrayList<>());
        }
        for (BasicBlock block : functionBasicBlocks) {
            for (Instruction instruction : block.getInstructions()) {
                if (!isFixedInstruction(instruction)) {
                    instruction.removeFromBasicBlock();
                    pendingInstructionsMap.get(getShallowestBlock.apply(earlyPlacement.get(instruction), latePlacement.get(instruction))).add(instruction);
                }
            }
        }
        placeBackInstructions(pendingInstructionsMap);
    }

    private void placeBackInstructions(Map<BasicBlock, List<Instruction>> placeConfig) {
        Stack<Instruction> readyInstructions = new Stack<>();
        var successors = new HashMap<Instruction, Set<Instruction>>();
        this.successors.forEach((instruction, list) -> successors.put(instruction, new HashSet<>(list)));
        placeConfig.forEach((basicBlock, list) -> {
            // 每条指令的后继包含设定基本块的终止指令，这样指令就会推迟到指定基本块再被放置
            for (Instruction instruction : list) {
                successors.get(instruction).add(basicBlock.getTerminateInstruction());
            }
        });
        // 指令放置完毕，更新localSuccessors
        Consumer<Instruction> onInstructionPlaced = placedInstruction -> {
            for (Instruction predecessor : predecessors.get(placedInstruction)) {
                if (!isFixedInstruction(predecessor)) {
                    var updatedSuccessors = successors.get(predecessor);
                    // 指令可能会多次限制同一个指令
                    // 但限制只要被移除一次，就会触发 readyInstructions.add
                    // 后续移除限制的时候，直接跳过
                    if (!updatedSuccessors.remove(placedInstruction)) continue;
                    if (updatedSuccessors.size() == 0) {
                        readyInstructions.add(predecessor);
                    }
                }
            }
        };
        // 递归搜索支配树，尝试尽晚放置指令
        var searcher = new Consumer<BasicBlock>() {
            @Override
            public void accept(BasicBlock basicBlock) {
                for (BasicBlock domSon : domTree.getDomSons(basicBlock)) {
                    accept(domSon);
                }
                // 取消基本块终止指令对放置在其中的指令的限制
                for (Instruction predecessor : placeConfig.get(basicBlock)) {
                    var updatedSuccessors = successors.get(predecessor);
                    updatedSuccessors.remove(basicBlock.getTerminateInstruction());
                    if (!isFixedInstruction(predecessor) && updatedSuccessors.size() == 0) {
                        readyInstructions.add(predecessor);
                    }
                }
                // 倒序遍历所有指令
                var instructionList = basicBlock.getInstructions();
                ListIterator<Instruction> iterator = instructionList.listIterator(instructionList.size());
                while (iterator.hasPrevious()) {
                    var instruction = iterator.previous();
                    // 如果是fixed指令，则立即安放该指令
                    if (isFixedInstruction(instruction)) {
                        onInstructionPlaced.accept(instruction);
                    }
                    // 非leading指令前可以安插其他指令
                    if (!BasicBlock.isLeadingInstruction(instruction)) {
                        // 插入顺序：s3,s2,s1,inst (s1指栈顶第一个指令)
                        var insertPoint = instruction;
                        while (!readyInstructions.isEmpty()) {
                            var puttingInstruction = readyInstructions.pop();
                            // 终止指令无法被 insertBefore 操作，需要改用其他方式插在主体指令的末尾
                            if (BasicBlock.isTerminateInstruction(insertPoint)) {
                                basicBlock.addInstruction(puttingInstruction);
                            } else {
                                puttingInstruction.insertBefore(insertPoint);
                            }
                            insertPoint = puttingInstruction;
                            onInstructionPlaced.accept(puttingInstruction);
                        }
                    }
                }
                for (Instruction instruction : placeConfig.get(basicBlock)) {
                    if (instruction.getBasicBlock() == null) {
                        throw new CompilationProcessCheckFailedException("Failed to place instruction back to block.");
                    }
                }
            }
        };
        searcher.accept(domTree.getDomRoot());
    }

    private void runGcm() {
        collectPredecessorsAndSuccessors();
        scheduleEarly();
        scheduleLate();
        replaceInstructions();
    }

    private static void runOnFunction(Function function) {
        var domTree = DomTree.buildOver(function);
        var loopForest = LoopForest.buildOver(function);
        Map<BasicBlock, Integer> loopDepthMap = new HashMap<>();
        Map<BasicBlock, Integer> domDepthMap = new HashMap<>();
        loopForest.getBasicBlockLoopMap().forEach((basicBlock, loop) -> loopDepthMap.put(basicBlock, loop.getLoopDepth()));
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            // 不在循环中的基本块，循环深度为-1
            if (!loopDepthMap.containsKey(basicBlock)) {
                loopDepthMap.put(basicBlock, -1);
            }
            // 生成基本块支配深度表
            domDepthMap.put(basicBlock, domTree.getDomDepth(basicBlock));
        }
        new GlobalCodeMotionPass(domTree, domDepthMap, loopDepthMap).runGcm();
    }

    /**
     * @param module 运行的模块。需要保证模块内没有无用代码。
     */
    public static void runOnModule(Module module) {
        DeadCodeEliminationPass.runOnModule(module);
        for (Function function : module.getFunctions()) {
            runOnFunction(function);
        }
    }

}
