package mid.Optimizer.Loop;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Function.LibFunction;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Instruction.Call;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.IntermediatePresentation.Instruction.Store;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.HyperParams;
import mid.Optimizer.Optimizer;
import mid.Optimizer.RedundancyElim.Straighten;
import mid.Optimizer.Utils.CloneManager;
import utils.tools.Timer;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;

public class LoopUnroll {
    /*
        会对循环次数为常值的SimplestLoop进行循环展开
        这当然就代表着，indvar的init和step都是常值了
        but问题是什么时候这个优化效果最好呢？
     */

    public void optimize() {
        boolean flag = true;
        while (flag && !Timer.INSTANCE.timeOut()) {
            flag = false;
            ArrayList<Loop> loops = Optimizer.instance().getLoopAnalyze().bfsLoops();
            Collections.reverse(loops);
            for (Loop loop : loops) {
                if (loop instanceof MinLoop minLoop) {
                    if (minLoop.getRepeatNumber() instanceof ConstNumber) {
                        flag = tryUnroll(minLoop);
                    }
                    if (flag) {
                        Optimizer.instance().dominAnalyze(minLoop.getHeader().getFunction());
                        (new Straighten()).optimize();
                        Optimizer.instance().getLoopAnalyze().analyze();
                        Optimizer.instance().loopOptAnalyze();
                        break;
                    }
                }
            }
        }
    }

    public void optimizeWitUAJ() {
        HashSet<Loop> loops = new HashSet<>(Optimizer.instance().getLoopAnalyze().getLoops());
        for (Loop loop : loops) {
            if (loop instanceof MinLoop minLoop) {
                if (minLoop.getRepeatNumber() instanceof ConstNumber) {
                    tryUnroll(minLoop);
                } else {
                    tryUnrollAndJam(minLoop);
                }
                Optimizer.instance().dominAnalyze(minLoop.getHeader().getFunction());
            }
        }
    }


    private boolean tryUnroll(MinLoop minLoop) {
        int n = ((ConstNumber) minLoop.getRepeatNumber()).getVal().intValue();
        int cnt = 0;
        for (BasicBlock block : minLoop.getBlocksInLoop()) {
            cnt += block.getInstructionList().size();
        }
        if (n == 0) return false;
        if (n == 1) {
            Br br = (Br) minLoop.getLatch().getLastInstruction();
            BasicBlock header = minLoop.getHeader();
            BasicBlock latch = minLoop.getLatch();
            BasicBlock exiting = minLoop.getExiting();
            if (exiting.equals(header)) {
                // 这种情况需要在latch之后加一个克隆的header
                return false;
            } else if (exiting.equals(latch)) {
                BasicBlock next = minLoop.getExit();
                for (Value v : br.getOperandList()) {
                    if (v instanceof BasicBlock && !v.equals(header)) {
                        next = (BasicBlock) v;
                    }
                }
                Br newBr = new Br(next);
                br.beReplacedBy(newBr);
                minLoop.getLatch().removeInstruction(br);
                br.destroy();
                minLoop.getLatch().addInstruction(newBr);
            } else {
                // 按理说不会有这种情况
                return false;
            }
            return true;
        }
        if (n > HyperParams.UNROLL_MAX_NUM ||
                minLoop.getHeader().getFunction().instructionCount() + cnt * n > HyperParams.INSTR_MAX_NUM ||
                minLoop.getBlocksInLoop().size() * n + minLoop.getHeader().getFunction().getBlocks().size()
                        > HyperParams.MAX_BLK_NUM_OPT) {
            return false;
        }
        if (minLoop.getBlocksInLoop().stream().allMatch(b ->
                b.getInstructionList().stream().anyMatch(i -> i instanceof Call call && call.getCallingFunction() instanceof LibFunction)
        )) {
            // 这种全是初始化的就不要展开了
            return false;
        }
        // 把latch和header合并，然后全部进行深克隆
        BasicBlock header = minLoop.getHeader();
        BasicBlock preHeader = minLoop.getPreheader();
        BasicBlock exiting = minLoop.getExiting();
        BasicBlock latch = minLoop.getLatch();
        BasicBlock exit = minLoop.getExit();

        // 依然是按照支配树遍历
        HashSet<BasicBlock> blocksInLoop = minLoop.getBlocksInLoop();
        ArrayList<BasicBlock> bfsDT = Optimizer.instance().bfsDominTreeArray(minLoop.getHeader());
        bfsDT.retainAll(blocksInLoop);
        // 1. 将phi指令替换为上一次展开中的对应值
        HashMap<Phi, Value> indPhiToVal = new HashMap<>();
        HashMap<Value, Phi> indValToPhi = new HashMap<>();
        for (Instruction instr : new ArrayList<>(header.getInstructionList())) {
            if (!(instr instanceof Phi phi)) {
                break;
            }
            // 由于是minLoop，因此lath是唯一的，所有的phi都可以直接用init替换
            Value indVal = phi.valueFromBlock(minLoop.getLatch());
            indPhiToVal.put(phi, indVal);
            indValToPhi.put(indVal, phi);
        }

        // 2. 去掉原本的exiting跳往exit的br，将latch改为跳到一个虚拟块latchNext
        Br exitBr = (Br) exiting.getLastInstruction();
        BasicBlock dest = exitBr.getIfTrue().equals(exit) ? exitBr.getIfFalse() : exitBr.getIfTrue();
        Br repBr = new Br(dest);
        exitBr.getBlock().addInstructionBeforeBranch(repBr);
        exitBr.getBlock().removeInstruction(exitBr);
        exitBr.destroy();

        BasicBlock latchNext = new BasicBlock();
        Br br = (Br) latch.getLastInstruction();
        Br latchNextBr = new Br(latchNext);
        br.beReplacedBy(latchNextBr);
        br.getBlock().addInstructionBeforeBranch(latchNextBr);
        br.getBlock().removeInstruction(br);
        br.destroy();

        // 3. 对改造后的循环内容块进行n-1次深克隆
        BasicBlock lastLatch = null;
        BasicBlock blockToBr = null;
        CloneManager cloneManager = new CloneManager(header.getFunction());
        cloneManager.put(latchNext, latchNext);
        for (int i = 0; i < n - 1; i++) {
            // 创建相应的克隆块
            cloneManager.put(exit, exit);
            for (BasicBlock block : bfsDT) {
                BasicBlock newBlock = new BasicBlock();
                cloneManager.put(block, newBlock);
                block.getFunction().addBlock(newBlock);
            }

            // 逐个克隆指令
            for (BasicBlock block : bfsDT) {
                BasicBlock newBlock = (BasicBlock) cloneManager.get(block);
                for (Instruction instr : block.getInstructionList()) {
                    if (indPhiToVal.containsKey(instr)) {
                        // 对phi指令，它应该是对应的上一个循环展开中的相应值
                        cloneManager.put(instr, indPhiToVal.get(instr));
                    } else {
                        Instruction newInstruction = cloneManager.getNewInstruction(instr);
                        newBlock.addInstruction(newInstruction);
                        cloneManager.put(instr, newInstruction);
                    }
                    if (indValToPhi.containsKey(instr)) {
                        indPhiToVal.put(indValToPhi.get(instr), cloneManager.get(instr));
                    }
                }
            }

            // 将上一个展开的latch跳转到本次展开的header
            if (lastLatch != null) {
                // 这里是为了保持模板的原貌，等到最后再处理
                lastLatch.redirectTo(latchNext, (BasicBlock) cloneManager.get(header));
            } else {
                blockToBr = (BasicBlock) cloneManager.get(header);
            }
            lastLatch = (BasicBlock) cloneManager.get(latch);
            cloneManager.cloneEnd();
        }
        ((Br) latch.getLastInstruction()).redirectTo(latchNext, blockToBr);
        BasicBlock lastExiting = (BasicBlock) cloneManager.get(exiting);
        lastExiting.addInstructionBeforeBranch(new Br(exit));

        //4. 对exit中，前部的phi中使用的循环内变量进行替换
        for (Instruction instruction : exit.getInstructionList()) {
            if (!(instruction instanceof Phi phi)) {
                break;
            }
            for (Value operand : phi.getOperandList()) {
                if (cloneManager.get(operand) != null) {
                    phi.replaceOperand(operand, cloneManager.get(operand));
                }
            }
        }

        //5. 删除header中的phi，替换为init
        for (Phi phi : indPhiToVal.keySet()) {
            phi.beReplacedBy(phi.valueFromBlock(preHeader));
            header.removeInstruction(phi);
            phi.destroy();
        }

        Optimizer.instance().getLoopAnalyze().removeLoop(minLoop);
        minLoop.destroy();
        return true;
    }

    private void tryUnrollAndJam(MinLoop minLoop) {
        /*
            和unroll不同，uaj需要修改归纳变量，并不破坏循环的标准结构
            循环结构改变：
            before ---  header->body->exiting->exit
            after  ---  header->body*4->exiting->remainPreHeader->remainHeader->remainBody->exit
                                    latch -> header
            归纳变量形式改变：
            只需要对原本的exiting/latch的分支br的cond进行克隆后的替换
         */

        // 把latch和header合并，然后全部进行深克隆
        BasicBlock header = minLoop.getHeader();
        BasicBlock preHeader = minLoop.getPreheader();
        BasicBlock exiting = minLoop.getExiting();
        BasicBlock latch = minLoop.getLatch();
        BasicBlock exit = minLoop.getExit();

        // 依然是按照支配树遍历
        HashSet<BasicBlock> blocksInLoop = minLoop.getBlocksInLoop();
        ArrayList<BasicBlock> bfsDT = Optimizer.instance().bfsDominTreeArray(minLoop.getHeader());
        bfsDT.retainAll(blocksInLoop);
        // 1. 将phi指令替换为上一次展开中的对应值
        HashMap<Phi, Instruction> indPhiToVal = new HashMap<>();
        HashMap<Instruction, Phi> indValToPhi = new HashMap<>();
        for (Instruction instr : new ArrayList<>(header.getInstructionList())) {
            if (!(instr instanceof Phi phi)) {
                break;
            }
            // 由于是minLoop，因此lath是唯一的，所有的phi都可以直接用init替换
            Instruction indVal = (Instruction) phi.valueFromBlock(minLoop.getLatch());
            indPhiToVal.put(phi, indVal);
            indValToPhi.put(indVal, phi);
        }

        // 2. 去掉原本的exiting跳往exit的br，将latch改为跳到一个虚拟块latchNext
        Br exitBr = (Br) exiting.getLastInstruction();
        BasicBlock dest = exitBr.getIfTrue().equals(exit) ? exitBr.getIfFalse() : exitBr.getIfTrue();
        Br repBr = new Br(dest);
        exitBr.getBlock().addInstructionBeforeBranch(repBr);
        exitBr.getBlock().removeInstruction(exitBr);
        exitBr.destroy();

        BasicBlock latchNext = new BasicBlock();
        Br latchBr = (Br) latch.getLastInstruction();
        Value cond = latchBr.getCond();
        Br latchNextBr = new Br(latchNext);
        latchBr.beReplacedBy(latchNextBr);
        latchBr.getBlock().addInstructionBeforeBranch(latchNextBr);
        latchBr.getBlock().removeInstruction(latchBr);

        // 3. 对改造后的循环内容块进行uajn+1次深克隆，最后一次作为remain循环
        BasicBlock lastLatch = null;
        BasicBlock latchToBr = null;
        CloneManager cloneManager = new CloneManager(header.getFunction());
        cloneManager.put(latchNext, latchNext);
        BasicBlock lastExiting = null;
        for (int i = 0; i < HyperParams.UNROLL_JAM_N; i++) {
            // 创建相应的克隆块
            cloneManager.put(exit, exit);
            for (BasicBlock block : bfsDT) {
                BasicBlock newBlock = new BasicBlock();
                cloneManager.put(block, newBlock);
                block.getFunction().addBlock(newBlock);
            }

            // 逐个克隆指令
            for (BasicBlock block : bfsDT) {
                BasicBlock newBlock = (BasicBlock) cloneManager.get(block);
                for (Instruction instr : block.getInstructionList()) {
                    Instruction newInstruction;
                    if (indPhiToVal.containsKey(instr)) {
                        // 对phi指令，它应该是对应的上一个循环展开中的相应值
                        newInstruction = indPhiToVal.get(instr);
                    } else {
                        newInstruction = cloneManager.getNewInstruction(instr);
                        newBlock.addInstruction(newInstruction);
                    }
                    cloneManager.put(instr, newInstruction);
                    if (indValToPhi.containsKey(instr)) {
                        indPhiToVal.put(indValToPhi.get(instr), newInstruction);
                    }
                }
            }

            // 将上一个展开的latch跳转到本次展开的header
            if (i < HyperParams.UNROLL_JAM_N - 1) {
                if (lastLatch != null) {
                    // 这里是为了保持模板的原貌，等到最后再处理
                    lastLatch.redirectTo(latchNext, (BasicBlock) cloneManager.get(header));
                } else {
                    latchToBr = (BasicBlock) cloneManager.get(header);
                }
            } else {
                // 应为unroll得到的循环的exiting设置跳转到本次循环的header
                lastExiting.addInstructionBeforeBranch(new Br((BasicBlock) cloneManager.get(header)));
            }
            lastLatch = (BasicBlock) cloneManager.get(latch);

            if (i == HyperParams.UNROLL_JAM_N - 2) {
                // 此时，所有的复制循环进行完毕，现在对其进行处理
                //4. 对最终的latch，将其修改为可以到达header的双目的br
                Br lastLatchBr = (Br) lastLatch.getLastInstruction();
                Br newLatchBr;
                if (latchBr.getOperandList().size() == 1) {
                    newLatchBr = new Br(header);
                } else if (latchBr.getIfTrue().equals(header)) {
                    newLatchBr = new Br(cloneManager.get(cond), header, latchNext);
                } else {
                    newLatchBr = new Br(cloneManager.get(cond), latchNext, header);
                }
                lastLatchBr.beReplacedBy(newLatchBr);
                lastLatch.addInstructionBeforeBranch(newLatchBr);
                lastLatch.deleteLastInstruction();
                lastLatchBr.destroy();
                lastExiting = (BasicBlock) cloneManager.get(exiting);
            }
            cloneManager.cloneEnd();
        }
        // 现在，可以对模板循环进行修改了
        latchNextBr.redirectTo(latchNext, latchToBr);
        // 为remain循环的latch设置到header的回边
        Br lastLatchBr = (Br) lastLatch.getLastInstruction();
        Br newLatchBr;
        BasicBlock remainHeader = (BasicBlock) cloneManager.get(header);
        if (latchBr.getOperandList().size() == 1) {
            newLatchBr = new Br(remainHeader);
        } else if (latchBr.getIfTrue().equals(header)) {
            newLatchBr = new Br(cond, remainHeader, lastLatchBr.getDest());
        } else {
            newLatchBr = new Br(cond, lastLatchBr.getDest(), remainHeader);
        }
        lastLatchBr.beReplacedBy(newLatchBr);
        lastLatch.addInstructionBeforeBranch(newLatchBr);
        lastLatch.deleteLastInstruction();
        lastLatchBr.destroy();
        latchBr.destroy();
        // 为remain循环设置到exit的出口
        lastExiting = (BasicBlock) cloneManager.get(exiting);
        lastExiting.addInstructionBeforeBranch(new Br(exit));

        //5. 对exit中，前部的phi中使用的循环内变量进行替换
        for (Instruction instruction : exit.getInstructionList()) {
            if (!(instruction instanceof Phi phi)) {
                break;
            }
            for (Value operand : phi.getOperandList()) {
                if (cloneManager.get(operand) != null) {
                    phi.replaceOperand(operand, cloneManager.get(operand));
                }
            }
        }

        //6. 删除header中的phi，替换为init
//        for (Phi phi : indPhiToVal.keySet()) {
//            phi.beReplacedBy(phi.valueFromBlock(preHeader));
//            header.removeInstruction(phi);
//            phi.destroy();
//        }
        Optimizer.instance().getLoopAnalyze().removeLoop(minLoop);
    }
}
