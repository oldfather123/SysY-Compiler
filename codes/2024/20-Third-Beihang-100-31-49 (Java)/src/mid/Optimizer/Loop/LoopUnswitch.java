package mid.Optimizer.Loop;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.HyperParams;
import mid.Optimizer.Optimizer;
import mid.Optimizer.RedundancyElim.DeadCode;
import mid.Optimizer.RedundancyElim.Straighten;
import mid.Optimizer.Utils.CloneManager;
import utils.tools.Timer;

import java.util.ArrayList;
import java.util.HashSet;

public class LoopUnswitch {

    public void optimize() {
        boolean flag;
        do {
            flag = tryUnswitch();
            if (flag) {
                Optimizer.instance().getLoopAnalyze().analyze();
                Optimizer.instance().loopOptAnalyze();
            }
        } while (flag && !Timer.INSTANCE.timeOut());
    }

    private boolean tryUnswitch() {
        for (Loop loop : Optimizer.instance().getLoopAnalyze().getLoops()) {
            /*
                 在lcssa中，是允许在exit的phi中直接使用值的，但是如果某个exit的父节点仍是exit，
                 那么它就无法保证仍可以使用（因为可能不支配），这个先暂时排除掉
            */
            boolean flag = false;
            for (BasicBlock exit : loop.getExits()) {
                HashSet<BasicBlock> parents = Optimizer.instance().getCFG().getParents(exit);
                if (parents.stream().anyMatch(blk -> !loop.getBlocksInLoop().contains(blk))) {
                    flag = true;
                    break;
                }
            }
            if (flag) {
                continue;
            }
            for (BasicBlock b : loop.getBlocksInLoop()) {
                Instruction i = b.getLastInstruction();
                if (i instanceof Br br && br.getOperandList().size() == 3) {
                    Value cond = br.getCond();
                    if (!(cond instanceof ConstNumber) && loop.isConstant(cond)) {
                        if (b.getFunction().getBlocks().size() + loop.getBlocksInLoop().size() >
                                HyperParams.MAX_BLK_NUM_OPT) {
                            continue;
                        }

                        unswitchFor(loop, br);
                        // 这里由于跳转的确定化可能产生一些不可达块
                        Optimizer.instance().getCFG().analyze(loop.getHeader().getFunction());
                        (new DeadCode()).removeUnreachableBlock();
                        Optimizer.instance().dominAnalyze(loop.getHeader().getFunction());
                        return true;
                    }
                }
            }
        }
        return false;
    }


    private void unswitchFor(Loop loop, Br br) {
        Function f = loop.getHeader().getFunction();
        BasicBlock ifTrueHeader = loop.getHeader();
        // 1. 复制loop
        CloneManager cloneManager = new CloneManager(f);

        ArrayList<BasicBlock> bfsDTArray = Optimizer.instance().bfsDominTreeArray(f.getEntranceBlock());
        bfsDTArray.retainAll(loop.getBlocksInLoop());
        for (BasicBlock block : bfsDTArray) {
            BasicBlock curBlock = new BasicBlock();
            cloneManager.put(block, curBlock);
            f.addBlock(curBlock);
        }

        for (BasicBlock block : bfsDTArray) {
            BasicBlock curBlock = (BasicBlock) cloneManager.get(block);
            for (Instruction instruction : block.getInstructionList()) {
                Instruction newInstruction = cloneManager.getNewInstruction(instruction);
                cloneManager.put(instruction, newInstruction);
                curBlock.addInstruction(newInstruction);
            }
        }
        cloneManager.cloneEnd();

        // 2. 修改preHeader的跳转
        BasicBlock preHeader = loop.getPreheader();
        Value cond = br.getCond();
        Br preHeaderBr = new Br(cond, ifTrueHeader, (BasicBlock) cloneManager.get(ifTrueHeader));
        Br prevPreHeaderBr = (Br) preHeader.getLastInstruction();
        preHeader.addInstructionBeforeBranch(preHeaderBr);
        preHeader.removeInstruction(prevPreHeaderBr);
        prevPreHeaderBr.destroy();

        for (Instruction i : ifTrueHeader.getInstructionList()) {
            if (!(i instanceof Phi phi)) {
                break;
            }
            ((Phi) cloneManager.get(phi)).addCond(phi.valueFromBlock(preHeader), preHeader);
        }

        // 3. 修改两侧的br为单目的
        Br ifTrueBr = new Br(br.getIfTrue());
        Br ifFalseBr = new Br((BasicBlock) cloneManager.get(br.getIfFalse()));
        BasicBlock brBlock = br.getBlock();
        brBlock.addInstructionBeforeBranch(ifTrueBr);
        brBlock.removeInstruction(br);
        br.beReplacedBy(ifTrueBr);
        br.destroy();

        brBlock = (BasicBlock) cloneManager.get(brBlock);
        brBlock.addInstructionBeforeBranch(ifFalseBr);
        br = (Br) cloneManager.get(br);
        brBlock.removeInstruction(br);
        br.beReplacedBy(ifFalseBr);
        br.destroy();

        // 4. 修改exits中的phi
        for (BasicBlock exit : loop.getExits()) {
            for (Instruction i : exit.getInstructionList()) {
                if (!(i instanceof Phi phi)) {
                    break;
                }
                for (Value operand : new ArrayList<>(phi.getOperandList())) {
                    if (operand instanceof BasicBlock b && loop.getExitings().contains(b)) {
                        phi.addCond(cloneManager.get(phi.valueFromBlock(b)),
                                (BasicBlock) cloneManager.get(b));
                    }
                }
            }
        }
    }

}
