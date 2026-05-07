package midend.optimism;

import midend.*;
import midend.Module;
import midend.analysis.Loop;
import midend.analysis.LoopInfo;
import midend.instr.BrInstr;
import midend.instr.Instruction;
import midend.instr.PhiInstr;

import java.util.ArrayList;

public class MergeBB {
    private Module module;

    public MergeBB(Module module) {
        this.module = module;
    }

    public void run() {
        mergeJumpOneBB();
        mergeIfJumpBB();
        mergeJumpOneBB();
    }

    public void mergeJumpOneBB() {
        boolean change = true;
        while (change) {
            change = false;
            for (Function function : module.getFunctions()) {
                ArrayList<BasicBlock> deleteBlocks = new ArrayList<>();
                for (BasicBlock block : function.getBlockList()) {
                    if (deleteBlocks.contains(block)) {
                        continue;
                    }
                    if (!(block.getInstrList().getLast() instanceof BrInstr)) {
                        continue;
                    }

                    BrInstr brInstr = (BrInstr) block.getInstrList().getLast();
                    if (brInstr.getIsIf()) {
                        continue;
                    }
                    BasicBlock nextBlock = brInstr.getDestBlock();
                    if (nextBlock.getInstrList().getFirst() instanceof PhiInstr) {
                        continue;
                    }
                    if (nextBlock.getPreBlock().size() > 1) {
                        continue;
                    }
                    deleteBlocks.add(nextBlock);
                    brInstr.remove();
                    for (Instruction instruction : nextBlock.getInstrList()) {
                        block.addInstr(instruction);
                        instruction.setBasicBlock(block);
                    }
                    //br and phi
                    //把所有br和phi指令中有nextblock的变为block
                    for (BasicBlock basicBlock : function.getBlockList()) {
                        if (basicBlock.getInstrList().getLast() instanceof BrInstr) {
                            ((BrInstr) basicBlock.getInstrList().getLast()).modifyBlock(nextBlock, block);
                        }
                        if (basicBlock.getInstrList().getFirst() instanceof PhiInstr) {
                            Instruction instruction = basicBlock.getInstrList().get(0);
                            int count = 0;
                            while (instruction instanceof PhiInstr) {
                                ((PhiInstr) instruction).modifyBlock(nextBlock, block);
                                instruction = basicBlock.getInstrList().get(++count);
                            }
                        }
                    }
                    //前驱后继
                    ArrayList<BasicBlock> nextBlocks = new ArrayList<>();
                    nextBlocks = nextBlock.getNextBlock();
                    block.setNextBlock(nextBlocks);
                    for (BasicBlock basicBlock : nextBlocks) {
                        int index = basicBlock.getPreBlock().indexOf(nextBlock);
                        if (index == -1) {
                            continue;
                        }
                        basicBlock.getPreBlock().set(index, block);
                    }
                    //设置exitBlock
                    if (function.getExitBlock().equals(nextBlock)) {
                        function.setExitBlock(block);
                    }

                    change = true;
                }
                for (BasicBlock block : deleteBlocks) {
                    function.getBlockList().remove(block);
                }
            }
        }
    }

    public void mergeIfJumpBB() {
        boolean change = true;
        while (change) {
            change = false;
            for (Function function : module.getFunctions()) {
                LoopInfo.loopAnalysis(function);
                ArrayList<BasicBlock> deleteBlocks = new ArrayList<>();
                for (BasicBlock block : function.getBlockList()) {
                    if (deleteBlocks.contains(block)) {
                        continue;
                    }
                    if (!(block.getInstrList().getLast() instanceof BrInstr)) {
                        continue;
                    }

                    BrInstr brInstr = (BrInstr) block.getInstrList().getLast();
                    if (!brInstr.getIsIf()) {
                        continue;
                    }
                    if (!(brInstr.getValue() instanceof Constant)) {
                        continue;
                    }
                    boolean in = false;
                    for (Loop loop : function.getTopLevelLoop()) {
                        if (loop.getHeader().equals(block)) {
                            in = true;
                            break;
                        }
                    }
                    if (in) {
                        continue;
                    }

//                    if (brInstr.getIfTrueBlock().getPreBlock().size() != 1 || brInstr.getIfTrueBlock().getInstrList().getFirst() instanceof PhiInstr) {
//                        continue;
//                    }
//                    if (brInstr.getIfFalseBlock().getPreBlock().size() != 1 || brInstr.getIfFalseBlock().getInstrList().getFirst() instanceof PhiInstr) {
//                        continue;
//                    }

                    BasicBlock nextBlock = ((ConstInt) brInstr.getValue()).getValue() == 1? brInstr.getIfTrueBlock() : brInstr.getIfFalseBlock();
                    if (nextBlock.getPreBlock().size() != 1 || nextBlock.getInstrList().getFirst() instanceof  PhiInstr) {
                        continue;
                    }
                    deleteBlocks.add(nextBlock);
                    brInstr.remove();
                    //BasicBlock nextBlock = ((ConstInt) brInstr.getValue()).getValue() == 1? brInstr.getIfTrueBlock() : brInstr.getIfFalseBlock();
                    for (Instruction instruction : nextBlock.getInstrList()) {
                        block.addInstr(instruction);
                        instruction.setBasicBlock(block);
                    }

                    //是否删除另一个块
                    BasicBlock otherBlock = ((ConstInt) brInstr.getValue()).getValue() == 0? brInstr.getIfTrueBlock() : brInstr.getIfFalseBlock();
                    Instruction instruction = otherBlock.getInstrList().getFirst();
                    int count = 0;
                    if (instruction instanceof PhiInstr) {
                        if (((PhiInstr) instruction).getPreBlockList().contains(block)) {
                            int index = ((PhiInstr) instruction).getPreBlockList().indexOf(block);
                            ((PhiInstr) instruction).getPreBlockList().remove(index);
                            while (instruction instanceof PhiInstr) {
//                                ((PhiInstr) instruction).removeValue(index);
//                                instruction = otherBlock.getInstrList().get(++count);

                                ((PhiInstr) instruction).removeValue(index);
                                int index0 = ((PhiInstr) instruction).getPreBlockList().indexOf(block);
                                if (index0 == -1) {
                                    instruction = otherBlock.getInstrList().get(++count);
                                    continue;
                                }
                                ((PhiInstr) instruction).getPreBlockList().remove(index0);
                                instruction = otherBlock.getInstrList().get(++count);
                            }
                        }
                    }
//                    while (instruction instanceof PhiInstr) {
//                        if (((PhiInstr) instruction).getPreBlockList().contains(block)) {
//                            ((PhiInstr) instruction).removeBlock(block);
//                        }
//                        instruction = otherBlock.getInstrList().get(++count);
//                    }

                    //br and phi
                    //把所有br和phi指令中有nextblock的变为block
                    for (BasicBlock basicBlock : function.getBlockList()) {
                        if (basicBlock.getInstrList().getLast() instanceof BrInstr) {
                            ((BrInstr) basicBlock.getInstrList().getLast()).modifyBlock(nextBlock, block);
                        }
                        if (basicBlock.getInstrList().getFirst() instanceof PhiInstr) {
                            Instruction instruction1 = basicBlock.getInstrList().get(0);
                            int count0 = 0;
                            while (instruction1 instanceof PhiInstr) {
                                ((PhiInstr) instruction1).modifyBlock(nextBlock, block);
                                instruction1 = basicBlock.getInstrList().get(++count0);
                            }
                        }
                    }
                    //前驱后继
                    ArrayList<BasicBlock> nextBlocks = new ArrayList<>();
                    nextBlocks = nextBlock.getNextBlock();
                    block.setNextBlock(nextBlocks);
                    for (BasicBlock basicBlock : nextBlocks) {
                        int index = basicBlock.getPreBlock().indexOf(nextBlock);
                        if (index == -1) {
                            continue;
                        }
                        basicBlock.getPreBlock().set(index, block);
                    }
                    //设置exitBlock
                    if (function.getExitBlock().equals(nextBlock)) {
                        function.setExitBlock(block);
                    }

                    change = true;
                }
                for (BasicBlock block : deleteBlocks) {
                    function.getBlockList().remove(block);
                }
            }
        }
    }
}
