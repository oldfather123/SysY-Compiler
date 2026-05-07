package mid.Optimizer.RedundancyElim;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.Alloca;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Instruction.Call;
import mid.IntermediatePresentation.Instruction.GlobalDecl;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Load;
import mid.IntermediatePresentation.Instruction.MoveIR;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.IntermediatePresentation.Instruction.Ret;
import mid.IntermediatePresentation.Instruction.Store;
import mid.IntermediatePresentation.Module;
import mid.IntermediatePresentation.User;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.Optimizer;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;

public class GCM {
    private final Module module;

    private final HashMap<Instruction, BasicBlock> earlySchedule = new HashMap<>();
    private final HashMap<Instruction, BasicBlock> lateSchedule = new HashMap<>();

    public GCM() {
        this.module = IRManager.getModule();
    }

    public void optimize() {
        scheduleEarly();
        scheduleLate();
        selectBlock();
    }

    private boolean isPinnedInstrs(Instruction instruction) {
        //包括phi,br,有副作用的call，ret
        //按理说move似乎应当是不需要pin的，但是似乎不pin也不能动，算了还是Pin了吧
        if (instruction instanceof Call call) {
            //只有没有副作用，不使用全局变量，非递归的函数才可以移动
            return call.GVNHash() == null;
        } else {
            return instruction instanceof Phi || instruction instanceof Br ||
                    instruction instanceof Load || instruction instanceof GlobalDecl ||
                    instruction instanceof Ret || instruction instanceof MoveIR ||
                    instruction instanceof Store || instruction instanceof Alloca;
        }
    }

    private void scheduleEarly() {
        for (Function f : module.getDecledFunctions()) {
            HashSet<Instruction> visited = new HashSet<>();
            for (BasicBlock bb : f.getBlocks()) {
                for (Instruction instr : bb.getInstructionList()) {
                    if (isPinnedInstrs(instr)) {
                        visited.add(instr);
                        earlySchedule.put(instr, instr.getBlock());
                        for (Value operand : instr.getOperandList()) {
                            if (operand instanceof Instruction instruction) {
                                scheduleEarlyForInstr(instruction, visited);
                            }
                        }
                    } else if (!visited.contains(instr)) {
                        //对于add 3,4这样的表达式，它是孤立的，如果被GVN移动到了错误的位置，就不能被GCM纠正
                        scheduleEarlyForInstr(instr, visited);
                    }
                }
            }
        }
    }

    private void scheduleEarlyForInstr(Instruction i, HashSet<Instruction> visited) {
        if (visited.contains(i)) {
            return;
        }
        if (isPinnedInstrs(i)) {
            earlySchedule.put(i, i.getBlock());
            visited.add(i);
            return;
        }
        visited.add(i);
        //从入口块开始
        earlySchedule.put(i, i.getBlock().getFunction().getEntranceBlock());
        HashMap<BasicBlock, Integer> depths = Optimizer.instance().getDominAnalyzer().
                getDominDepths(i.getBlock().getFunction());

        for (Value operand : i.getOperandList()) {
            if (operand instanceof Instruction x && !(x instanceof GlobalDecl)) {
                scheduleEarlyForInstr(x, visited);
                if (depths.get(earlySchedule.get(i)) < depths.get(earlySchedule.get(x))) {
                    earlySchedule.replace(i, earlySchedule.get(x));
                }
            }
        }
    }

    private void scheduleLate() {
        for (Function f : module.getDecledFunctions()) {
            HashSet<Instruction> visited = new HashSet<>();
            for (BasicBlock bb : f.getBlocks()) {
                for (Instruction instr : bb.getInstructionList()) {
                    if (isPinnedInstrs(instr)) {
                        visited.add(instr);
                        for (Value user : instr.getUserList()) {
                            if (user instanceof Instruction instruction) {
                                scheduleLateForInstruction(instruction, visited);
                            }
                        }
                    } else if (!visited.contains(instr)) {
                        //对于add 3,4这样的表达式，它是孤立的，如果被GVN移动到了错误的位置，就不能被GCM纠正
                        scheduleLateForInstruction(instr, visited);
                    }
                }
            }
        }

    }

    private void scheduleLateForInstruction(Instruction i, HashSet<Instruction> visited) {
        if (visited.contains(i) || isPinnedInstrs(i)) {
            return;
        }
        visited.add(i);
        lateSchedule.put(i, null);
        for (User user : i.getUserList()) {
            if (user instanceof Instruction y && !(y instanceof GlobalDecl)) {
                scheduleLateForInstruction(y, visited);
                BasicBlock use = earlySchedule.get(y);
                if (y instanceof Phi phi) {
                    //可能出现phi [t5, b1], [t5, b2]的情况，因为之前做了GVN，所以要求的应该是这些块共同的lca
                    for (BasicBlock block : phi.getSrcBlockWhen(i)) {
                        BasicBlock lca = lateSchedule.get(i);
                        lateSchedule.put(i, Optimizer.instance().getDominAnalyzer().LCAInDominTree(lca, block));
                    }
                } else {
                    BasicBlock lca = lateSchedule.get(i);
                    lateSchedule.put(i, Optimizer.instance().getDominAnalyzer().LCAInDominTree(lca, use));
                }
            }
        }
    }

    private void selectBlock() {
        for (Function f : module.getDecledFunctions()) {
            HashMap<BasicBlock, ArrayList<Instruction>> originInstrList = new HashMap<>();
            for (BasicBlock bb : f.getBlocks()) {
                originInstrList.put(bb, new ArrayList<>(bb.getInstructionList()));
            }

            for (BasicBlock bb : f.getBlocks()) {
                ArrayList<Instruction> instructions = originInstrList.get(bb);
                for (Instruction instr : instructions) {
                    if (!earlySchedule.containsKey(instr) || !lateSchedule.containsKey(instr) ||
                            isPinnedInstrs(instr)) {
                        continue;
                    }
                    BasicBlock originBlock = instr.getBlock();
                    BasicBlock best = lateSchedule.get(instr);
                    BasicBlock lca = lateSchedule.get(instr);
                    if (lca == null) {
                        continue;
                    }
                    while (lca != null) {
                        if (lca.getLoopDepth() < best.getLoopDepth()) {
                            best = lca;
                        }
                        if (best.getLoopDepth() == 0 || lca.equals(earlySchedule.get(instr))) {
                            break;
                        }
                        lca = Optimizer.instance().getDominAnalyzer().getIDominer(lca);
                    }

                    //由于originBlock被GVN移动过，它不一定是对的
                    originBlock.removeInstruction(instr);
                    //这里要考虑，加到这个基本块的哪里？
                    putIntoBlock(instr, best);
//                    best.addInstrAtEntry(instr);
                }
            }
        }
    }

    private void putIntoBlock(Instruction instr, BasicBlock block) {
        //在所有use之前，所有operand的def之后
        ArrayList<Value> operands = instr.getOperandList();
        ArrayList<Instruction> instructions = new ArrayList<>(block.getInstructionList());
        int lateIndex = -1;
        //最后一个要留给跳转
        int earlyIndex = instructions.size() - 1;
        for (Instruction instruction : instructions) {
            if (instruction instanceof Phi) {
                continue;
            }
            if (lateIndex == -1 && instr.getUserList().contains(instruction)) {
                lateIndex = instructions.indexOf(instruction);
            }
            if (operands.contains(instruction)) {
                earlyIndex = instructions.indexOf(instruction) + 1;
            }
        }
        if (lateIndex != -1) {
            if (earlyIndex - 1 >= 0 &&
                    operands.contains(instructions.get(earlyIndex - 1)) && lateIndex < earlyIndex) {
                // 可能出现的最晚index小于最早的，不存在交集，表明需要重新分析
                Instruction user = instructions.get(lateIndex);
                Instruction operand = instructions.get(earlyIndex - 1);
                block.getInstructionList().set(earlyIndex - 1, user);
                block.getInstructionList().set(lateIndex, operand);
                putIntoBlock(instr, block);
            } else {
                block.addInstructionAt(lateIndex, instr);
            }
        } else {
            block.addInstructionAt(earlyIndex, instr);
        }
    }
}
