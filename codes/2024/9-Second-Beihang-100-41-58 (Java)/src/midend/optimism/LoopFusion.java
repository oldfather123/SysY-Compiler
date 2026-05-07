package midend.optimism;

import Utils.ImmutablePair;
import midend.*;
import midend.LLVMType.UndefinedType;
import midend.Module;
import midend.analysis.Loop;
import midend.analysis.LoopInfo;
import midend.instr.*;

import java.util.ArrayList;
import java.util.HashMap;

public class LoopFusion {
    private Module module;

    public LoopFusion(Module module) {
        this.module = module;
    }

    //循环融合条件：
    //1.两个循环必须相邻
    //2.两个循环必须只有一个单一入口和单一出口（无break和return）
    //3.没有负距离依赖项
    //while (i < n) {
    //  a[i] = temp[i] + 2;
    //  i = i + 1;
    //}
    //while (i < n) {
    //  b[i] = a[i + 1] + a[i];
    //  i = i + 1;
    //}
    public void run() {
        for (Function function : module.getFunctions()) {
            LoopInfo.loopAnalysis(function);
            LoopInfo.getExitingBlocks();
            LoopInfo.iteratorAnalysis(function);
            ArrayList<ImmutablePair<Loop, Loop>> loopPairs = new ArrayList<>();
            for (Loop loop : function.getLoops()) {
                for (Loop loop2 : function.getLoops()) {
                    if (loop2.equals(loop)) {
                        continue;
                    }
                    if (canFusion(loop, loop2)) {
                        loopPairs.add(new ImmutablePair<>(loop, loop2));
                    }
                }
            }

            for (ImmutablePair<Loop, Loop> loops : loopPairs) {
                Loop left = loops.left();
                Loop right = loops.right();
                ArrayList<BasicBlock> blocks2 = new ArrayList<>(right.getBasicBlocks());
                blocks2.remove(0);
                BasicBlock returnBlock = null;
                for (BasicBlock block : right.getHeader().getPreBlock()) {
                    if (right.getBasicBlocks().contains(block)) {
                        returnBlock = block;
                    }
                }

                ArrayList<Instruction> addInstr = new ArrayList<>();
//                for (BasicBlock block : blocks2) {
//                for (Instruction instruction : returnBlock.getInstrList()) {
//                    if (!instruction.equals(right.getItAlu()) && !instruction.equals(returnBlock.getInstrList().getLast())) {
//                        addInstr.add(instruction);
//                    }
//                }
//                }
                BasicBlock moveBlock = ((ALUInstr) left.getItAlu()).getBasicBlock();
                int index = moveBlock.getInstrList().indexOf(left.getItAlu());
                int blockIndex = function.getBlockList().indexOf(moveBlock);

                BasicBlock itBlock = new BasicBlock(UndefinedType.undefined, function);
                itBlock.addInstr((Instruction) left.getItAlu());
                itBlock.addInstr(moveBlock.getInstrList().getLast());
                ((BrInstr) left.getExitBlock().get(0).getInstrList().getLast()).modifyBlock(right.getHeader(), right.getExitBlock().get(0));
//                ((BrInstr) left.getHeader().getInstrList().getLast()).modifyBlock(left.getExitBlock().get(0), right.getExitBlock().get(0));
                moveBlock.getInstrList().remove(left.getItAlu());
                moveBlock.getInstrList().remove(moveBlock.getInstrList().getLast());
                BrInstr brInstr = new BrInstr(blocks2.get(0));
                brInstr.setBasicBlock(moveBlock);
                moveBlock.addInstr(brInstr);
                function.getBlockList().add(blockIndex + 1, itBlock);

                function.getBlockList().removeAll(right.getBasicBlocks());
                for (int count = blocks2.size() - 1; count >= 0; count--) {
                    function.getBlockList().add(blockIndex + 1, blocks2.get(count));
                }

                for (Instruction instruction : returnBlock.getInstrList()) {
                    if (instruction.equals(right.getItAlu()) || instruction.equals(returnBlock.getInstrList().getLast())) {
                        addInstr.add(instruction);
                    }
//                    instruction.getBasicBlock().getInstrList().remove(instruction);
//                    instruction.setBasicBlock(moveBlock);
//                    moveBlock.getInstrList().add(index, instruction);
                }
                for (Instruction instruction : addInstr) {
                    instruction.remove();
                }
                addInstr.clear();
                BrInstr brInstr1 = new BrInstr(itBlock);
                brInstr1.setBasicBlock(returnBlock);
                returnBlock.addInstr(brInstr1);
                for (Instruction instruction : right.getExitBlock().get(0).getInstrList()) {
                    if (instruction instanceof PhiInstr) {
                        ((PhiInstr) instruction).modifyBlock(right.getHeader(), left.getExitBlock().get(0));
                    }
                }

                for (Instruction instruction : right.getHeader().getInstrList()) {
                    if (instruction instanceof PhiInstr && !instruction.equals(right.getItVar())) {
                        addInstr.add(instruction);
                    }
                }
                for (Instruction instruction : addInstr) {
                    instruction.getBasicBlock().getInstrList().remove(instruction);
                    instruction.setBasicBlock(left.getHeader());
                    left.getHeader().getInstrList().addFirst(instruction);
                    ((PhiInstr) instruction).setPreBlockList(left.getHeader().getPreBlock());
                }
                Value itVar2 = right.getItVar();
                itVar2.replaceUse(left.getItVar());
//                function.getBlockList().remove(right.getBasicBlocks());
            }
        }
    }

    public boolean canFusion(Loop loop, Loop loop2) {
        if (loop.getSubLoops().size() != 0 || loop2.getSubLoops().size() != 0) {
            return false;
        }
        ArrayList<BasicBlock> exit = loop.getExitBlock();
        ArrayList<BasicBlock> exit2 = loop2.getExitBlock();
        //单一出口
        if (exit.size() != 1 || exit2.size() != 1) {
            return false;
        }
        //必须相邻
        BasicBlock exit1 = exit.get(0);
        if (!exit1.getNextBlock().contains(loop2.getHeader())) {
            return false;
        }
        //单一入口
        if (loop.getHeader().getPreBlock().size() != 2 || loop2.getHeader().getPreBlock().size() != 2) {
            return false;
        }

        if (loop.getItBegin() == null || loop2.getItBegin() == null || loop.getItEnd() == null || loop2.getItEnd() == null || loop.getItChange() == null || loop2.getItChange() == null) {
            return false;
        }
        if (!(loop.getItBegin().equals(loop2.getItBegin()) || (loop.getItBegin() instanceof ConstInt && loop2.getItBegin() instanceof ConstInt && ((ConstInt) loop.getItBegin()).getValue() == ((ConstInt) loop2.getItBegin()).getValue()))
                || !(loop.getItEnd().equals(loop2.getItEnd()) || (loop.getItEnd() instanceof ConstInt && loop2.getItEnd() instanceof ConstInt && ((ConstInt) loop.getItEnd()).getValue() == ((ConstInt) loop2.getItEnd()).getValue()))
                || !(loop.getItChange().equals(loop2.getItChange()) || (loop.getItChange() instanceof ConstInt && loop2.getItChange() instanceof ConstInt && ((ConstInt) loop.getItChange()).getValue() == ((ConstInt) loop2.getItChange()).getValue()))) {
            return false;
        }

        Value itVar = loop.getItVar();
        Value itVar2 = loop2.getItVar();
        int count = 0;
        for (User user : itVar2.getUserList()) {
            if (user instanceof ALUInstr && !(user instanceof CmpInstr)) {
                count++;
            }
            if (count > 1) {
                return false;
            }
        }
        count = 0;
        for (User user : itVar.getUserList()) {
            if (user instanceof ALUInstr && !(user instanceof CmpInstr)) {
                count++;
            }
            if (count > 1) {
                return false;
            }
        }

        for (Instruction instruction : exit1.getInstrList()) {
            if (instruction instanceof CallInstr && ((CallInstr) instruction).getFunction().isSideEffect()) {
                return false;
            }
            if (instruction instanceof StoreInstr) {
                return false;
            }
        }

        return true;
    }
}
