package cn.edu.bit.newnewcc.pass.ir;

import cn.edu.bit.newnewcc.ir.Module;
import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.type.VoidType;
import cn.edu.bit.newnewcc.ir.value.BasicBlock;
import cn.edu.bit.newnewcc.ir.value.Function;
import cn.edu.bit.newnewcc.ir.value.Instruction;
import cn.edu.bit.newnewcc.ir.value.instruction.CallInst;
import cn.edu.bit.newnewcc.ir.value.instruction.MemoryInst;
import cn.edu.bit.newnewcc.ir.value.instruction.PhiInst;
import cn.edu.bit.newnewcc.pass.ir.structure.DomTree;
import cn.edu.bit.newnewcc.pass.ir.util.PureFunctionDetect;

import java.util.*;

/**
 * 指令重排 <br>
 * 重新排序指令，使得活跃变量区间总长度尽可能短，方便寄存器分配 <br>
 */
public class InstructionSchedulePass {

    private static class BasicBlockProcessor {
        private final BasicBlock basicBlock;
        private final Set<Instruction> exitLiveInstructions;
        private final Set<Function> pureFunctions;

        private BasicBlockProcessor(BasicBlock basicBlock, Set<Instruction> exitLiveInstructions, Set<Function> pureFunctions) {
            this.basicBlock = basicBlock;
            this.exitLiveInstructions = exitLiveInstructions;
            this.pureFunctions = pureFunctions;
        }

        // 对于最后一次使用在当前块内的变量，记录其目前还被哪些指令使用，以便在剩余单个使用时为对应指令加分
        private final Map<Instruction, Set<Instruction>> liveValueUseSet = new HashMap<>();

        // 指令依赖图：记录每个指令依赖哪些当前块内的前置语句
        private final Map<Instruction, Set<Instruction>> dependencyUseeMap = new HashMap<>();
        // 指令依赖图：记录每个指令被哪些后续语句依赖
        private final Map<Instruction, Set<Instruction>> dependencyUserMap = new HashMap<>();

        // 收集新增的活跃变量，记录这些变量被哪些语句使用
        private void buildLiveValueUseSet() {
            for (Instruction instruction : basicBlock.getMainInstructions()) {
                for (Operand operand : instruction.getOperandList()) {
                    if (operand.getValue() instanceof Instruction usedInstruction) {
                        if (!exitLiveInstructions.contains(usedInstruction)) {
                            if (!liveValueUseSet.containsKey(usedInstruction)) {
                                liveValueUseSet.put(usedInstruction, new HashSet<>());
                            }
                            liveValueUseSet.get(usedInstruction).add(instruction);
                        }
                    }
                }
            }
        }

        // 建立指令依赖图
        private void buildInstructionDependencyMap() {
            for (Instruction instruction : basicBlock.getMainInstructions()) {
                dependencyUseeMap.put(instruction, new HashSet<>());
                dependencyUserMap.put(instruction, new HashSet<>());
            }
            Set<Instruction> previousInstructions = new HashSet<>();
            Instruction lastFixedInstruction = null;
            for (Instruction instruction : basicBlock.getMainInstructions()) {
                // 依赖本基本块内产生的值
                for (Operand operand : instruction.getOperandList()) {
                    if (operand.getValue() instanceof Instruction usedInstruction &&
                            previousInstructions.contains(usedInstruction)) {
                        dependencyUseeMap.get(instruction).add(usedInstruction);
                        dependencyUserMap.get(usedInstruction).add(instruction);
                    }
                }
                // 固定顺序的指令，依次依赖前序值
                if (isFixedInstruction(instruction)) {
                    if (lastFixedInstruction != null) {
                        dependencyUseeMap.get(instruction).add(lastFixedInstruction);
                        dependencyUserMap.get(lastFixedInstruction).add(instruction);
                    }
                    lastFixedInstruction = instruction;
                }
                // 维护本基本块已经产生的值
                previousInstructions.add(instruction);
            }
        }

        // 重排基本思路：
        // 为每个指令打分，分低的指令优先安放
        // 分数是动态的。若指令是一个活跃变量的最后一次使用，-1分。若指令产生了一个活跃变量，+1分。
        // 打分规则可以改（是超参数（怎么又炼起丹了）），找到结果最好的即可。
        private final Map<Instruction, Integer> instructionScoreMap = new HashMap<>();

        private record PendingInstruction(Instruction instruction,
                                          int score, int timestamp) implements Comparable<PendingInstruction> {
            @Override
            public int compareTo(PendingInstruction o) {
                if (score != o.score) return Integer.compare(score, o.score);
                if (timestamp != o.timestamp) return Integer.compare(timestamp, o.timestamp);
                return 0;
            }
        }

        private final Set<Instruction> pendingInstructions = new HashSet<>();
        private final PriorityQueue<PendingInstruction> pendingInstructionQueue = new PriorityQueue<>();

        private int timeStamp = Integer.MAX_VALUE;

        private int getTimeStamp() {
            return timeStamp--;
        }

        private void addInstructionAsPending(Instruction instruction) {
            pendingInstructionQueue.add(new PendingInstruction(instruction, instructionScoreMap.get(instruction), getTimeStamp()));
            pendingInstructions.add(instruction);
        }

        private void updateScore(Instruction instruction, int scoreDiff) {
            int oldScore = instructionScoreMap.get(instruction);
            instructionScoreMap.put(instruction, oldScore + scoreDiff);
            if (pendingInstructions.contains(instruction)) {
                pendingInstructionQueue.add(new PendingInstruction(instruction, instructionScoreMap.get(instruction), getTimeStamp()));
            }
        }

        private void onInstructionPlaced(Instruction instruction) {
            // 维护 liveValueUseSet
            for (Operand operand : instruction.getOperandList()) {
                if (operand.getValue() instanceof Instruction usedInstruction) {
                    if (liveValueUseSet.containsKey(usedInstruction)) {
                        var users = liveValueUseSet.get(usedInstruction);
                        users.remove(instruction);
                        for (Instruction user : users) {
                            updateScore(user, -1);
                        }
                    }
                }
            }
            // 维护 dependencyUseeMap
            for (Instruction useeInstruction : dependencyUseeMap.get(instruction)) {
                var users = dependencyUserMap.get(useeInstruction);
                users.remove(instruction);
                if (users.size() == 0) {
                    addInstructionAsPending(useeInstruction);
                }
            }
        }

        private Instruction getNextInstruction() {
            while (!pendingInstructionQueue.isEmpty()) {
                var pendingInstruction = pendingInstructionQueue.remove();
                if (pendingInstruction.score != instructionScoreMap.get(pendingInstruction.instruction)) continue;
                if (!pendingInstructions.contains(pendingInstruction.instruction)) continue;
                pendingInstructions.remove(pendingInstruction.instruction);
                return pendingInstruction.instruction;
            }
            return null;
        }

        private void rescheduleInstructions() {
            // 收集新增的活跃变量，记录这些变量被哪些语句使用
            buildLiveValueUseSet();
            // 建立指令依赖图
            buildInstructionDependencyMap();
            // 执行重排过程
            for (Instruction instruction : basicBlock.getMainInstructions()) {
                // 移除重排的指令
                instruction.removeFromBasicBlock();
                // 给予所有指令基本分数
                instructionScoreMap.put(instruction, 0);
                // 加入初始可用的指令
                if (dependencyUserMap.get(instruction).size() == 0) {
                    addInstructionAsPending(instruction);
                }
                // 产生活跃变量的减1分
                if (instruction.getType() != VoidType.getInstance()) {
                    updateScore(instruction, -1);
                }
            }
            // 扫描活跃变量，检查是否存在可以加分的活跃变量
            liveValueUseSet.forEach((instruction, users) -> {
                updateScore(users.iterator().next(), 1);
            });
            // 更新 exitLiveInstructions ，将本基本块内用到的外部值加入其中，并删除已经在本基本块内定义的值
            exitLiveInstructions.addAll(liveValueUseSet.keySet());
            for (Instruction instruction : basicBlock.getInstructions()) {
                // set的特性：如果不存在就不移除，不需要额外检查
                exitLiveInstructions.remove(instruction);
            }
            // 放回指令
            Instruction lastPlacedInstruction = null;
            while (true) {
                var instruction = getNextInstruction();
                if (instruction == null) break;
                if (lastPlacedInstruction == null) {
                    basicBlock.addInstruction(instruction);
                } else {
                    instruction.insertBefore(lastPlacedInstruction);
                }
                lastPlacedInstruction = instruction;
                onInstructionPlaced(instruction);
            }
        }

        /**
         * 判断一个指令是否需要被固定调用顺序 <br>
         * 需要固定调用顺序的指令包括：内存指令，Call指令 <br>
         *
         * @param instruction 被判断的指令
         * @return 需要固定返回true；否则返回false。
         */
        private boolean isFixedInstruction(Instruction instruction) {
            if (instruction instanceof MemoryInst) return true;
            if (instruction instanceof CallInst callInst) {
                if (callInst.getCallee() instanceof Function function && pureFunctions.contains(function)) return false;
                else return true;
            }
            return false;
        }

        public static void process(BasicBlock basicBlock, Set<Instruction> liveInstructions, Set<Function> pureFunctions) {
            new BasicBlockProcessor(basicBlock, liveInstructions, pureFunctions).rescheduleInstructions();
        }
    }

    private final Function function;
    private final DomTree domTree;
    private final Set<Function> pureFunctions;

    private InstructionSchedulePass(Function function, Set<Function> pureFunctions) {
        this.function = function;
        this.domTree = DomTree.buildOver(function);
        this.pureFunctions = pureFunctions;
    }

    // 将 phi 指令的 use 挂载到对应基本块的末尾
    private final Map<BasicBlock, Set<Instruction>> phiUsages = new HashMap<>();

    /**
     * 将 phi 指令的 use 挂载到对应基本块的末尾
     */
    private void relocatePhiUses() {
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            phiUsages.put(basicBlock, new HashSet<>());
        }
        for (BasicBlock basicBlock : function.getBasicBlocks()) {
            for (Instruction leadingInstruction : basicBlock.getLeadingInstructions()) {
                if (leadingInstruction instanceof PhiInst phiInst) {
                    phiInst.forEach((entry, value) -> {
                        if (value instanceof Instruction usedInstruction) {
                            phiUsages.get(basicBlock).add(usedInstruction);
                        }
                    });
                }
            }
        }
    }

    /**
     * 在支配树上dfs，按dfs序逆序依次处理基本块
     *
     * @param block 待排序的基本块
     * @return 基本块use的外部指令的集合
     */
    // 按dfs序逆序处理基本块，是因为这恰好符合“基本块use的外部指令的集合”的计算顺序
    private Set<Instruction> dfsDomTree(BasicBlock block) {
        // 收集的出口活跃变量
        // 先添加本基本块的结尾phi使用，和终止指令的使用（这两个动不了）
        Set<Instruction> liveInstructions = new HashSet<>(phiUsages.get(block));
        for (Operand operand : block.getTerminateInstruction().getOperandList()) {
            if (operand.getValue() instanceof Instruction instruction) {
                liveInstructions.add(instruction);
            }
        }
        // 递归收集其他的出口活跃变量
        for (BasicBlock domSon : domTree.getDomSons(block)) {
            var sonLiveInstructions = dfsDomTree(domSon);
            // 启发式合并，保证算法的复杂度
            if (sonLiveInstructions.size() > liveInstructions.size()) {
                sonLiveInstructions.addAll(liveInstructions);
                liveInstructions = sonLiveInstructions;
            } else {
                liveInstructions.addAll(sonLiveInstructions);
            }
        }
        // 重排当前基本块的指令
        BasicBlockProcessor.process(block, liveInstructions, pureFunctions);
        return liveInstructions;
    }

    private void runOnFunction() {
        relocatePhiUses();
        dfsDomTree(domTree.getDomRoot());

    }

    public static void runOnModule(Module module) {
        var pureFunctions = PureFunctionDetect.getPureFunctions(module);
        for (Function function : module.getFunctions()) {
            new InstructionSchedulePass(function, pureFunctions).runOnFunction();
        }
    }
}
