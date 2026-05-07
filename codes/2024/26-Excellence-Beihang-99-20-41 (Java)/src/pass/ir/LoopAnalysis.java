package pass.ir;

import driver.Config;
import ir.IrModule;
import ir.value.BasicBlock;
import ir.value.Function;
import pass.Pass;
import pass.utils.ControlFlowGraph;
import pass.utils.DominatorTree;
import pass.utils.LoopInfo;

import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Stack;

public class LoopAnalysis implements Pass.IrPass {
    private final HashMap<BasicBlock, LoopInfo> loopInfoMap = new HashMap<>();
    private final List<LoopInfo> loops = new java.util.ArrayList<>();
    private final List<LoopInfo> allLoops = new java.util.ArrayList<>();

    @Override
    public void run(IrModule module) {
        for (Function function : module.getFunctions()) {
            loopInfoMap.clear();
            loops.clear();
            allLoops.clear();
            runLoopAnalysis(function);

            if (Config.DebugSettings.displayLoopInfo) {
                System.err.println("Loop analysis for function " + function.getName() + " done.");
                System.err.println("Loops: " + loops.size());
                for (var loop : loops) {
                    System.err.println(loop);
                }
                System.err.println("All loops: " + allLoops.size());
                for (var loop : allLoops) {
                    System.err.println(loop);
                }
            }
        }
    }

    private void runLoopAnalysis(Function function) {
        DominatorTree dominatorTree = function.getDomTree();
        ControlFlowGraph cfg = function.getCFG();
        List<BasicBlock> postOrder = dominatorTree.getPostOrderIdom();

        for (var header : postOrder) {
            // System.err.println("Header: " + header);
            Stack<BasicBlock> backEdges = new Stack<>();
            for (var block : cfg.getPredecessors(header)) {
                // System.err.println("  " + header + " -> " + block);
                if (dominatorTree.isAncestor(header, block)) {
                    backEdges.push(block);
                }
            }
            if (!backEdges.empty()) {
                detectLoop(header, backEdges, cfg);
            }
        }

        for (var block : postOrder) {
            fillLoopInfo(block);
        }

        tidyUpAllLoops();
        function.setLoopInfoMap(new HashMap<>(loopInfoMap));
        function.setLoops(new LinkedList<>(loops));
        function.setAllLoops(new LinkedList<>(allLoops));

        for (LoopInfo loop : this.allLoops) {
            for (BasicBlock block : loop.getBlocks()) {
                for (BasicBlock successor : cfg.getSuccessors(block)) {
                    if (!loop.getBlocks().contains(successor)) {
                        loop.addExitBlock(successor);
                        loop.addExitingBlock(block);
                    }
                }
            }
            for (BasicBlock predecessor : cfg.getPredecessors(loop.getHeaderBlock())) {
                if (loop.getBlocks().contains(predecessor)) {
                    loop.addLatchBlock(predecessor);
                }
            }
        }
    }

    private void detectLoop(BasicBlock header, Stack<BasicBlock> backEdges, ControlFlowGraph cfg) {
        LoopInfo currentLoop = new LoopInfo(header);
        while (!backEdges.empty()) {
            BasicBlock backEdge = backEdges.pop();
            LoopInfo subLoop = loopInfoMap.get(backEdge);
            if (subLoop == null) {
                loopInfoMap.put(backEdge, currentLoop);
                if (backEdge == currentLoop.getHeaderBlock()) {
                    continue;
                }
                for (var edgePredecessor : cfg.getPredecessors(backEdge)) {
                    backEdges.push(edgePredecessor);
                }
            } else {
                while (subLoop.hasParent()) {
                    subLoop = subLoop.getParentLoop();
                }
                if (subLoop == currentLoop) {
                    continue;
                }
                subLoop.setParentLoop(currentLoop);
                for (var subLoopHeaderPredecessor : cfg.getPredecessors(subLoop.getHeaderBlock())) {
                    if (loopInfoMap.get(subLoopHeaderPredecessor) != subLoop) {
                        backEdges.push(subLoopHeaderPredecessor);
                    }
                }
            }
        }
    }

    private void fillLoopInfo(BasicBlock block) {
        LoopInfo subLoop = loopInfoMap.get(block);
        if (subLoop != null && subLoop.getHeaderBlock() == block) {
            if (subLoop.hasParent()) {
                subLoop.getParentLoop().addSubLoop(subLoop);
            } else {
                loops.add(subLoop);
            }
            subLoop.reverseBlock();
            subLoop.reverseSubLoop();
            subLoop = subLoop.getParentLoop();
        }
        while (subLoop != null) {
            subLoop.addBlock(block);
            subLoop = subLoop.getParentLoop();
        }
    }

    private void tidyUpAllLoops() {
        Stack<LoopInfo> queue = new Stack<>();
        queue.addAll(loops);
        allLoops.addAll(loops);
        while (!queue.isEmpty()) {
            var loop = queue.pop();
            if (!loop.getSubLoops().isEmpty()) {
                queue.addAll(loop.getSubLoops());
                allLoops.addAll(loop.getSubLoops());
            }
        }
    }

    @Override
    public String getName() {
        return "loop-analysis";
    }
}
