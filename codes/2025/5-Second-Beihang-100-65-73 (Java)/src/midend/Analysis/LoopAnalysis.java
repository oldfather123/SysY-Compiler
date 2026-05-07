package midend.Analysis;

import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.CFG;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * 前置 CFG
 */
public class LoopAnalysis {
    private static int loopCounter = 0; // 身份标识

    public static void execute(List<Function> functions) {
        for (Function func : functions) {
            loopInfoAnalyse(func);
        }

    }

    public static int getAndAddLoopCounter() {
        return loopCounter++;
    }

    public static void loopInfoAnalyse(Function function) {
        List<BasicBlock> post = CFG.getDomPostOrder(function);
        for (BasicBlock b : post) {
            b.initLoopInfo();
        }
        Set<LoopInfo> ancientLoops = new HashSet<>();
        HashSet<BasicBlock> jumpFrom = new HashSet<>();
        for (BasicBlock block : post) {
            Set<BasicBlock> domSet = block.getDomSet();
            jumpFrom.clear();
            for (BasicBlock pre : block.getPres()) {
                if (domSet.contains(pre)) {
                    jumpFrom.add(pre);
                }
            }
            if (!jumpFrom.isEmpty()) {
                LoopInfo ancientLoop = new LoopInfo(0, block, null);
                ancientLoops.add(ancientLoop);
                subLoopIdentify(ancientLoop, jumpFrom, ancientLoops);
            }
        }
        for (LoopInfo loop : ancientLoops) {
            enterExitAnalysis(loop);
            sisterAnalyse(loop);
            loopDepthFlush(loop);
        }

        function.setAncientLoopInfo(ancientLoops);
    }

    private static void subLoopIdentify(LoopInfo loop, Set<BasicBlock> jumpFrom, Set<LoopInfo> ancientLoops) {
        loop.addLatchBlocks(jumpFrom);
        ArrayList<BasicBlock> workSet = new ArrayList<>(jumpFrom);
        HashSet<BasicBlock> visited = new HashSet<>();
        while (!workSet.isEmpty()) {
            BasicBlock bb = workSet.removeLast();
            if (visited.contains(bb)) {
                continue;
            }
            visited.add(bb);

            if (bb.getLoopBelonged() == null) {
                bb.setLoopBelonged(loop);
                loop.addBodyBlock(bb);
                if (bb == loop.getLoopHeader()) {
                    continue;
                }
            } else {
                LoopInfo ancientLoop = bb.getLoopBelonged().getAncientLoop();
                if (ancientLoop != loop) {
                    loop.addChildLoop(ancientLoop);
                    ancientLoop.setParentLoop(loop);
                    ancientLoops.remove(ancientLoop);
                } else {
                    continue;
                }
                bb = ancientLoop.getLoopHeader();
            }
            workSet.addAll(bb.getPres());
        }
    }

    private static void enterExitAnalysis(LoopInfo loop) {
        for (LoopInfo childLoop : loop.getChildLoop()) {
            enterExitAnalysis(childLoop);
        }

        HashSet<BasicBlock> allBlocksInLoop = new HashSet<>();
        loop.getAllBlocks(allBlocksInLoop);
        for (BasicBlock pre : loop.getLoopHeader().getPres()) {
            if (!allBlocksInLoop.contains(pre)) {
                loop.addEnterFrom(pre);
            }
        }
        for (BasicBlock bb : allBlocksInLoop) {
            bb.getSucs().stream()
                    .filter(suc -> !allBlocksInLoop.contains(suc))
                    .forEach(suc -> {
                        loop.addExiting(bb);
                        loop.addExitTarget(suc);
                        suc.isExitOflLoop.add(loop);
                    });
        }
    }

    private static void loopDepthFlush(LoopInfo loop) {
        if (loop.getParentLoop() == null) {
            loop.setLoopDepth(1);
        } else {
            loop.setLoopDepth(loop.getParentLoop().getLoopDepth() + 1);
        }

        for (LoopInfo childLoop : loop.getChildLoop()) {
            loopDepthFlush(childLoop);
        }

    }

    private static void sisterAnalyse(LoopInfo loop) {
        for (LoopInfo childLoop : loop.getChildLoop()) {
            sisterAnalyse(childLoop);
        }
        BasicBlock sisterHeader = loop.getLoopHeader().sisterLoopHeader;
        if (sisterHeader != null) {
            if (sisterHeader.sisterLoopHeader == loop.getLoopHeader()) {
                if (loop.getLoopHeader().isElder) {
                    loop.tailLoop = sisterHeader.getLoopBelonged();
                }
            }
        }
    }
}