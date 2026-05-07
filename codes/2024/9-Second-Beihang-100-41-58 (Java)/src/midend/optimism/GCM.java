package midend.optimism;

import midend.*;
import midend.LLVMType.VoidType;
import midend.Module;
import midend.analysis.CFGBuilder;
import midend.analysis.LoopInfo;
import midend.instr.*;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.LinkedList;

public class GCM {
    private Module module;
    private HashSet<Instruction> visited = new HashSet<>();

    public GCM(Module module) {
        this.module = module;
    }

    public void run() {
        for (Function function : module.getFunctions()) {
            runGCM(function);
        }
    }

    public void runGCM(Function function) {
        if (function.getBlockList().size() == 1) {
            return;
        }
        CFGBuilder.initFunc(function);
//        CFGBuilder.getCFG(function);
        CFGBuilder.getDom(function);
        CFGBuilder.getImmDom(function);
        LoopInfo.loopAnalysis(function);
        ArrayList<BasicBlock> postorder = CFGBuilder.getDomPostOrder(function);
        Collections.reverse(postorder);
        visited = new HashSet<>();
        for (BasicBlock block : postorder) {
            LinkedList<Instruction> instructions = new LinkedList<>(block.getInstrList());
            for (Instruction instruction : instructions) {
                if (isPinned(instruction)) {
                    visited.add(instruction);
                    for (Value input : instruction.getValueList()) {
                        if (input instanceof Instruction) {
                            schedule_early((Instruction) input, function);
                        }
                    }
                }
            }
        }

        visited = new HashSet<>();

        for (BasicBlock block : function.getBlockList()) {
            LinkedList<Instruction> instructions = new LinkedList<>(block.getInstrList());
            for (Instruction instruction : instructions) {
                schedule_late(instruction, function);
            }
        }
    }

    public void schedule_late(Instruction instruction, Function function) {
        if (visited.contains(instruction) || isPinned(instruction)) {
            return;
        }
//        if (instruction.getName().equals("%reg131")) {
//            System.out.println("");
//        }
        visited.add(instruction);
        BasicBlock LCA = null;
        //addBB
        for (User user : instruction.getUserList()) {
            if (user instanceof Instruction) {
                schedule_late((Instruction) user, function);
                BasicBlock use = ((Instruction) user).getBasicBlock();
                if (user instanceof PhiInstr) {
                    use = ((PhiInstr) user).getPreBlock(instruction);
//                    for (int count = 0; count < ((PhiInstr) user).getPreBlockList().size(); count++) {
//                        if (user.getValue(count) instanceof Instruction && user.getValue(count).equals(instruction)) {
//                            use = ((PhiInstr) user).getPreBlock(count);
//                        }
//                    }
                }
                LCA = findLCA(LCA, use);
            }
        }
        //TODO:LCA==null?
        if (!instruction.getUserList().isEmpty()) {
            BasicBlock best = LCA;
            while (!(LCA.equals(instruction.getBasicBlock()))) {
                // if lca.loop_nest < best.loop_nest
//                if (LCA.getName().equals("block5")){
//                    System.out.println("");
//                }
                if (LCA.getLoopDepth() < best.getLoopDepth()) {
                    best = LCA;
                }
                LCA = LCA.getParentBlock();
            }

            instruction.getBasicBlock().getInstrList().remove(instruction);
            best.getInstrList().add(best.getInstrList().size() - 1, instruction);
            instruction.setBasicBlock(best);
        }

        BasicBlock block = instruction.getBasicBlock();
        for (int count = 0; count < block.getInstrList().size(); count++) {
            Instruction instruction1 = block.getInstrList().get(count);
            if (!instruction1.equals(instruction) && instruction1.getValueList().contains(instruction) && !(instruction1 instanceof PhiInstr)) {
//                block.getInstrList().remove(instruction);
//                block.getInstrList().add(Math.max(0, count - 1), instruction);
                block.getInstrList().add(count, instruction);
                block.getInstrList().remove(block.getInstrList().size() - 2);
                break;
            }
        }

    }

    public BasicBlock findLCA(BasicBlock LCA, BasicBlock use) {
        if (LCA == null) {
            return use;
        } else if (LCA.getDomDepth() < use.getDomDepth()) {
            int size = use.getDomDepth() - LCA.getDomDepth();
            for (int count = 0; count < size; count++) {
                use = use.getParentBlock();
            }
        } else if (LCA.getDomDepth() > use.getDomDepth()) {
            int size = LCA.getDomDepth() - use.getDomDepth();
            for (int count = 0; count < size; count++) {
                LCA = LCA.getParentBlock();
            }
        }
        while (!(LCA.equals(use))) {
            LCA = LCA.getParentBlock();
            use = use.getParentBlock();
        }
        return LCA;
    }

    public void schedule_early(Instruction instruction, Function function) {
        if (visited.contains(instruction) || isPinned(instruction)) {
            return;
        }
        visited.add(instruction);
        //i.block = root
        BasicBlock root = function.getBlockList().get(0);
        instruction.getBasicBlock().getInstrList().remove(instruction);
        instruction.setBasicBlock(root);
        root.getInstrList().add(root.getInstrList().size() - 1, instruction);

        for (Value input : instruction.getValueList()) {
            if (input instanceof Instruction) {
                schedule_early((Instruction) input, function);
                if (instruction.getBasicBlock().getDomDepth() < ((Instruction) input).getBasicBlock().getDomDepth()) {
                    BasicBlock xBlock = ((Instruction) input).getBasicBlock();
                    instruction.getBasicBlock().getInstrList().remove(instruction);
                    instruction.setBasicBlock(xBlock);
                    xBlock.getInstrList().add(xBlock.getInstrList().size() - 1, instruction);
                }
            }
        }

    }

    public boolean isPinned(Instruction instruction) {
        return instruction instanceof PhiInstr || instruction instanceof BrInstr ||
                instruction instanceof RetInstr || instruction instanceof StoreInstr ||
                instruction instanceof CallInstr || instruction instanceof LoadInstr;
    }
}
