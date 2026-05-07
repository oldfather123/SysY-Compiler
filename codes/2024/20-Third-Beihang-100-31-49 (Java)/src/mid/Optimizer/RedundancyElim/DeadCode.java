package mid.Optimizer.RedundancyElim;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Instruction.Call;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.IntermediatePresentation.Instruction.Ret;
import mid.IntermediatePresentation.Instruction.Store;
import mid.IntermediatePresentation.Module;
import mid.IntermediatePresentation.User;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.ControllFlow.ControlFlowGraph;
import mid.Optimizer.Loop.ScEvValue;
import mid.Optimizer.Optimizer;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.LinkedList;

public class DeadCode {
    private final Module module;
    private ControlFlowGraph CFG;

    public DeadCode() {
        module = IRManager.getModule();
        CFG = Optimizer.instance().getCFG();
    }

    public boolean optimize() {
        //去除掉流图中没有父母的不可达块
        boolean changed;
        CFG = Optimizer.instance().getCFG();
        changed = removeUnreachableBlock();
        removeUnusedInstructions();
        changed |= removeEmptyBlocks();
        return changed;
    }

    public void clearLoopInfo() {
        for (ScEvValue scEvValue : Optimizer.instance().getLoopAnalyze().getScEvValues()) {
            scEvValue.destroy();
        }
        Optimizer.instance().getLoopAnalyze().getScEvValues().clear();
    }

    public void scanJump() {
        //删除基本块跳转指令之后的指令，暂不考虑全局关系
        CFG = Optimizer.instance().getCFG();
        module.getDecledFunctions().forEach(x -> x.getBlocks().forEach(this::scanJumpForBlock));
    }

    private void scanJumpForBlock(BasicBlock block) {
        boolean reachEnd = false;
        ArrayList<Instruction> instructions = new ArrayList<>();
        for (Instruction instruction : block.getInstructionList()) {
            if (!reachEnd) {
                instructions.add(instruction);
                if (instruction instanceof Ret || instruction instanceof Br) {
                    reachEnd = true;
                }
            } else {
                instruction.destroy();
            }
        }
        block.updataInstructionList(instructions);
    }

    public boolean removeUnreachableBlock() {
        boolean flag = false;
        for (Function f : module.getDecledFunctions()) {
            flag |= removeUnreachableBlockForFunction(f);
        }
        if (flag) {
            CFG.analyze();
        }
        return flag;
    }

    private boolean removeUnreachableBlockForFunction(Function function) {
        HashSet<BasicBlock> blocks = new HashSet<>(function.getBlocks());
        boolean hasChanged;
        boolean flag = false;
        do {
            HashSet<BasicBlock> reachedBlocks = CFG.reachedBlocks(function);

            hasChanged = !blocks.equals(reachedBlocks);
            if (hasChanged) {
                flag = true;
                ArrayList<BasicBlock> unReachedBlocks = new ArrayList<>(function.getBlocks());
                unReachedBlocks.removeAll(reachedBlocks);
                unReachedBlocks.forEach(BasicBlock::destroy);
                function.getBlocks().retainAll(reachedBlocks);
                function.getOperandList().retainAll(reachedBlocks);
                blocks = new HashSet<>(function.getBlocks());
            }
        } while (hasChanged);
        return flag;
    }

    private void removeUnusedInstruction() {
        module.getDecledFunctions().forEach(x -> x.getBlocks().forEach(this::removeUnusedInstructionForBlock));
    }

    private void removeUnusedInstructionForBlock(BasicBlock block) {
        ArrayList<Instruction> instructions = new ArrayList<>(block.getInstructionList());
        boolean hasChanged;
        do {
            hasChanged = false;
            for (Instruction instruction : instructions) {
                if (instruction.isUseless()) {
                    hasChanged = true;
                    block.removeInstruction(instruction);
                    //调用destroy方法将其在def-use链中去掉
                    instruction.destroy();
                }
            }
            instructions = new ArrayList<>(block.getInstructionList());
        } while (hasChanged);
    }

    public void removeUnusedInstructions() {
        //首先求出一个有用指令的闭包
        //"有用"的指令包括：store ret br call
        HashSet<Instruction> usefulInstrs = new HashSet<>();
        for (Function f : module.getDecledFunctions()) {
            for (BasicBlock b : f.getBlocks()) {
                for (Instruction i : b.getInstructionList()) {
                    if (i instanceof Br || i instanceof Ret || i instanceof Store) {
                        usefulInstrs.add(i);
                    } else if (i instanceof Call call && Optimizer.instance().hasSideEffect(call.getCallingFunction())) {
                        usefulInstrs.add(i);
                    }
                }
            }
        }

        //根据闭包扩大有用指令集并删除无用指令
        LinkedList<Instruction> queue = new LinkedList<>(usefulInstrs);
        while (!queue.isEmpty()) {
            Instruction i = queue.poll();
            if (i instanceof Phi phi) {
                usefulInstrs.addAll(phi.getMoveIrs());
                queue.addAll(phi.getMoveIrs());
            }
            for (Value operand : i.getOperandList()) {
                if (operand instanceof Instruction opIns && !usefulInstrs.contains(operand)) {
                    usefulInstrs.add(opIns);
                    queue.add(opIns);
                }
            }
        }

        //删除无用指令
        for (Function f : module.getDecledFunctions()) {
            for (BasicBlock b : f.getBlocks()) {
                ArrayList<Instruction> instructions = new ArrayList<>(b.getInstructionList());
                for (Instruction instruction : instructions) {
                    if (!usefulInstrs.contains(instruction)) {
                        b.removeInstruction(instruction);
                        instruction.destroy();
                    }
                }
            }
        }
    }

    private boolean removeEmptyBlocks() {
        boolean hasChanged = false;
        for (Function f : module.getDecledFunctions()) {
            ArrayList<BasicBlock> emptyBlocks = new ArrayList<>();
            for (BasicBlock b : f.getBlocks()) {
                /*
                    1. 只有一个跳转指令 (如果是ret，当然也是不行的)
                    2. 不能是入口块
                    3. 没有任何phi指令用到了这个块
                 */
                if (b.getInstructionList().size() == 1 && !b.equals(f.getEntranceBlock()) &&
                        b.getInstructionList().get(0) instanceof Br br && br.getDest() != null) {
                    boolean isUseless = true;
                    for (User user : b.getUserList()) {
                        if (user instanceof Phi) {
                            isUseless = false;
                            break;
                        }
                    }
                    if (isUseless) {
                        emptyBlocks.add(b);
                        hasChanged = true;
                    }
                }
            }

            for (BasicBlock b : emptyBlocks) {
                Br br = (Br) b.getInstructionList().get(0);
                b.beReplacedBy(br.getDest());
                b.destroy();
                f.removeBlock(b);
            }
        }

        if (hasChanged) {
            removeEmptyBlocks();
            return true;
        }
        return false;
    }
}
