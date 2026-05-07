package mid.Optimizer.Loop;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.Optimizer.Optimizer;
import mid.Optimizer.RedundancyElim.DeadCode;

import java.util.Date;
import java.util.HashSet;
import java.util.LinkedList;

public class LoopSimplify {
    HashSet<Loop> loops;

    public void optimize() {
        loops = Optimizer.instance().getLoopAnalyze().getLoops();
        // 1. 消除不可达块，防止出现循环内部的块（除header外）有不属于循环的前继
        (new DeadCode()).removeUnreachableBlock();
        // 2. 构造单一的preheader
        loops.forEach(this::insertPreheader);
        loops.forEach(Loop::reAnalyzeBlocksInside);
        // 3. 构造专一的exit
        loops.forEach(this::insertDedicatedExit);
        // 4. 去掉多于一个的latch
        killMultipleLatches();
        loops.forEach(Loop::reAnalyzeBlocksInside);
        // 5. 最后重新分析循环结构
        Optimizer.instance().getLoopAnalyze().analyze();
        Optimizer.instance().getCFG().analyze();
        loops.forEach(this::analyze);
    }

    private void insertPreheader(Loop loop) {
        BasicBlock header = loop.getHeader();
        HashSet<BasicBlock> predcessors = new HashSet<>(Optimizer.instance().getCFG().getParents(header));
        predcessors.removeAll(loop.getBlocksInLoop());
        if (predcessors.size() == 1) {
            loop.setPreheader((BasicBlock) predcessors.toArray()[0]);
        }
        HashSet<BasicBlock> blocksInLoop = loop.getBlocksInLoop();
        predcessors.removeAll(blocksInLoop);
        // if predcessors are empty?

        BasicBlock preheader = new BasicBlock();
        preheader.addInstruction(new Br(header));
        header.getFunction().addBlock(preheader);
        Optimizer.instance().splitBlockPredcessors(header, preheader, predcessors);
        loop.setPreheader(preheader);
    }

    private void insertDedicatedExit(Loop loop) {
        HashSet<BasicBlock> blocksInLoop = loop.getBlocksInLoop();
        HashSet<BasicBlock> exits = new HashSet<>();
        HashSet<BasicBlock> exitings = new HashSet<>();

        for (BasicBlock block : blocksInLoop) {
            HashSet<BasicBlock> children = new HashSet<>(Optimizer.instance().getCFG().getChildren(block));
            children.removeAll(blocksInLoop);
            if (children.size() > 0) {
                exitings.add(block);
            }
            exits.addAll(children);
        }

        HashSet<BasicBlock> newExits = new HashSet<>();
        for (BasicBlock exit : exits) {
            HashSet<BasicBlock> innerEntry = new HashSet<>(blocksInLoop);
            HashSet<BasicBlock> parent = new HashSet<>(Optimizer.instance().getCFG().getParents(exit));
            innerEntry.retainAll(parent);
            if (innerEntry.equals(parent)) {
                // header一定存在循环外的入口，而exit则不一定
                newExits.add(exit);
                continue;
            }

            BasicBlock newExit = new BasicBlock();
            newExit.addInstruction(new Br(exit));
            loop.getHeader().getFunction().addBlock(newExit);
            // 将循环内部的跳转转到newExit块上
            Optimizer.instance().splitBlockPredcessors(exit, newExit, innerEntry);
            newExits.add(newExit);
        }
        newExits.forEach(loop::addExit);
        exitings.forEach(loop::addExiting);
    }

    private void killMultipleLatches() {
        boolean willRestart = false;
        for (Loop loop : loops) {
            HashSet<BasicBlock> latches = loop.getLatchs();
            BasicBlock header = loop.getHeader();
            if (latches.size() > 1 && latches.size() < 8) {
                willRestart = separateNestedLoop(loop);
                if (willRestart) {
                    break;
                } else {
                    // 插入新的latch，构造单一的backedge
                    BasicBlock newLatch = new BasicBlock();
                    newLatch.addInstruction(new Br(header));
                    header.getFunction().addBlock(newLatch);
                    Optimizer.instance().splitBlockPredcessors(header, newLatch, latches);
                    loop.setLatch(newLatch);
                }
            }
        }

        if (willRestart) {
            // 如果存在被拆分的内循环，则重新进行此方法
            killMultipleLatches();
        }
    }

    private boolean separateNestedLoop(Loop loop) {
        BasicBlock header = loop.getHeader();
        Phi nestedPhi = null;
        // 找到内循环的Phi
        for (Instruction instruction : header.getInstructionList()) {
            if (!(instruction instanceof Phi phi)) {
                continue;
            }
            if (phi.getOperandList().contains(phi)) {
                nestedPhi = phi;
                break;
            }
        }

        if (nestedPhi == null) {
            return false;
        } else {
            // 将来自内循环内外的前继分离
            BasicBlock latch = (BasicBlock) nestedPhi.getOperandList().get(
                    nestedPhi.getOperandList().indexOf(nestedPhi) + 1);
            HashSet<BasicBlock> predcessors = new HashSet<>(Optimizer.instance().getCFG().getParents(header));
            predcessors.remove(loop.getPreheader());
            predcessors.removeAll(Optimizer.instance().getCFG().mayPassingBy(header, latch));

            // precessors是来自外循环的所有跳转块，为外循环设置新的header
            BasicBlock newHeader = new BasicBlock();
            header.getFunction().addBlock(newHeader);
            newHeader.addInstruction(new Br(header));
            Optimizer.instance().splitBlockPredcessors(header, newHeader, predcessors);

            // 构造新的循环，更新loops
            loops.remove(loop);
            Loop innerLoop = new Loop(header);
            innerLoop.addLatch(latch);
            insertDedicatedExit(innerLoop);
            Loop outterLoop = new Loop(newHeader);
            HashSet<BasicBlock> outterLatches = new HashSet<>(loop.getLatchs());
            outterLatches.remove(latch);
            outterLatches.forEach(outterLoop::addLatch);
            insertDedicatedExit(outterLoop);

            return true;
        }
    }

    private void analyze(Loop loop) {
        // preheader
        BasicBlock header = loop.getHeader();
        HashSet<BasicBlock> predcessors = new HashSet<>(Optimizer.instance().getCFG().getParents(header));
        predcessors.removeAll(loop.getBlocksInLoop());
        loop.setPreheader((BasicBlock) predcessors.toArray()[0]);

        // exit
        HashSet<BasicBlock> blocksInLoop = loop.getBlocksInLoop();
        HashSet<BasicBlock> exits = new HashSet<>();
        HashSet<BasicBlock> exitings = new HashSet<>();

        for (BasicBlock block : blocksInLoop) {
            HashSet<BasicBlock> children = new HashSet<>(Optimizer.instance().getCFG().getChildren(block));
            children.removeAll(blocksInLoop);
            if (children.size() > 0) {
                exitings.add(block);
            }
            exits.addAll(children);
        }
        exits.forEach(loop::addExit);
        exitings.forEach(loop::addExiting);
    }
}
