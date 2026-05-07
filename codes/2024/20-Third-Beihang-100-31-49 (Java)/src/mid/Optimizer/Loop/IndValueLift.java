package mid.Optimizer.Loop;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.ALU;
import mid.IntermediatePresentation.Instruction.Call;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.IntermediatePresentation.Instruction.Store;
import mid.IntermediatePresentation.User;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.Optimizer;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

public class IndValueLift {
    public void optimize() {
        new HashSet<>(Optimizer.instance().getLoopAnalyze().getLoops()).forEach(this::optimizeFor);
    }

    private void optimizeFor(Loop loop) {
        /*
            识别loop中的所有IndValue
            1. 如果未在外部使用，则可以考虑是否能删除
            2. 如果在外部使用，则可以考虑能否通过Exiting Analyze直接求出退出值
         */
        HashMap<Value, ScEvValue> bivs = loop.getInds();
        HashSet<BasicBlock> blocksInLoop = loop.getBlocksInLoop();
        HashSet<Value> unUsedBivs = new HashSet<>();
        for (Value biv : bivs.keySet()) {
            boolean unUsed = true;
            if (biv.getUserList().size() > 1) {
                continue;
            }
            for (User user : biv.getUserList()) {
                if (user instanceof Instruction && !blocksInLoop.contains(user.getBlock())) {
                    unUsed = false;
                    break;
                }
            }
            if (unUsed) {
                unUsedBivs.add(biv);
            }
        }


        for (Value biv : unUsedBivs) {
            if (biv.getUserList().size() == 1 && unUsedBivs.contains(biv.getUserList().get(0)) ||
                    biv.getUserList().size() == 0) {
                // <=1 是因为删除的过程中，可能将phi先删掉
                Instruction bivInstr = (Instruction) biv;
                bivInstr.getBlock().removeInstruction(bivInstr);
                loop.removeInd(bivInstr);
                bivInstr.destroy();
            }
        }


        if (!(loop instanceof MinLoop minLoop)) {
            return;
        }
        // 如果是minLoop，可以考虑更激进的优化
        /*
            考虑一个minLoop，可以根据repeatNumber为所有使用到的ind求值
            所求的ind为{init, +, step}，所以得到的值是init+step*（repeatNumber-1）
         */
        Value repeatNumber = minLoop.getRepeatNumber();
        boolean changed = false;
        for (Value biv : bivs.keySet()) {
            ScEvValue scev = loop.getInds().get(biv);
            Instruction bivIncVal = new ALU(scev.getStepVal(), "*", repeatNumber, true);
            minLoop.getExit().addInstructionBeforeBranch(bivIncVal);
            Instruction bivFinalVal = new ALU(scev.getInitVal(), "+", bivIncVal, true);
            minLoop.getExit().addInstructionBeforeBranch(bivFinalVal);
            bivFinalVal = new ALU(bivFinalVal, "-", scev.getStepVal(), true);
            minLoop.getExit().addInstructionBeforeBranch(bivFinalVal);
            for (User user : new ArrayList<>(biv.getUserList())) {
                if (user instanceof Instruction instr && !blocksInLoop.contains(user.getBlock())) {
                    // LCSSA之后，这里的user一定是phi指令；由于是minLoop，更进一步一定是单phi指令，直接替换即可
                    user.beReplacedBy(bivFinalVal);
                    user.getBlock().removeInstruction(instr);
                    user.destroy();
                    changed = true;
                }
            }
        }


        /*
            对于没有在外部被使用的biv，考虑其在循环内部的user
            构造一个在循环外部被使用了的value集合（以及call&store），由这一个基础集合迭代得到所有"有用"的集合
            如果没有，直接用preheader跳到exit，然后把循环内所有指令和基本块都删去
            由于是minLoop，因此不需要考虑phi的问题
         */

        // 构造有用集合
        HashSet<Instruction> usefulInstrs = new HashSet<>();
        for (BasicBlock block : blocksInLoop) {
            for (Instruction instr : block.getInstructionList()) {
                boolean isUseful = (instr instanceof Call || instr instanceof Store);
                for (User user : instr.getUserList()) {
                    if (!blocksInLoop.contains(user.getBlock())) {
                        isUseful = true;
                        break;
                    }
                }
                if (isUseful) {
                    usefulInstrs.add(instr);
                }
            }
        }

        if (usefulInstrs.isEmpty()) {
            removeEmptyLoop(minLoop);
            minLoop.getBlocksInLoop().forEach(BasicBlock::destroy);
        } else if (changed) {
            // 如果进行了归纳变量的替换，则重新进行优化以尝试删去无用的归纳变量
            optimizeFor(loop);
        }
    }

    private void removeEmptyLoop(MinLoop minLoop) {
        for (Loop loop : new HashSet<>(Optimizer.instance().getLoopAnalyze().getSubLoops(minLoop))) {
            Optimizer.instance().getLoopAnalyze().removeLoop(loop);
            loop.destroy();
        }
        BasicBlock preheader = minLoop.getPreheader();
        preheader.redirectTo(minLoop.getHeader(), minLoop.getExit());
        Function f = minLoop.getPreheader().getFunction();
        for (BasicBlock b : minLoop.getBlocksInLoop()) {
            if (!f.getBlocks().contains(b)) {
                continue;
            }
            for (Loop lp : Optimizer.instance().getLoopAnalyze().getLoops()) {
                if (!lp.equals(minLoop)) {
                    lp.getBlocksInLoop().remove(b);
                    if (lp.getExitings().contains(b)) {
                        lp.getExitings().remove(b);
                        if (lp.getBlocksInLoop().contains(preheader)) {
                            lp.getExitings().add(preheader);
                        }
                    }
                    if (lp instanceof MinLoop mlp && mlp.getExiting().equals(b)) {
                        mlp.setExiting(preheader);
                    }
                }
            }
            b.beReplacedBy(preheader);
            f.removeBlock(b);
        }
        Optimizer.instance().getLoopAnalyze().removeLoop(minLoop);
        minLoop.destroy();
    }
}
