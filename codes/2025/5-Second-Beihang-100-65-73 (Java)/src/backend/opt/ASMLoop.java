package backend.opt;

import backend.asm.structure.ASMBasicBlock;
import frontend.ir.structure.BasicBlock;
import midend.Analysis.LoopInfo;

import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class ASMLoop {
    private final ASMBasicBlock preHeader;  // 有且只有一个支配所有循环体块的、只执行一次的块
    private final Set<ASMBasicBlock> bodyBlocks; // only this level
    private final Set<ASMLoop> childLoopSet;

    private ASMLoop(ASMBasicBlock preHeader) {
        this.preHeader = preHeader;
        this.bodyBlocks = new HashSet<>();
        this.childLoopSet = new HashSet<>();
    }

    public static ASMLoop initLoop(LoopInfo midLoop, Map<BasicBlock, ASMBasicBlock> basicBlockMap) {
        ASMLoop newLoop = new ASMLoop(basicBlockMap.get(midLoop.getPreHeader()));
        for (BasicBlock basicBlock : midLoop.getBodyBlocks()) {
            newLoop.bodyBlocks.add(basicBlockMap.get(basicBlock));
        }
        for (LoopInfo midChildLoop : midLoop.getChildLoop()) {
            newLoop.childLoopSet.add(initLoop(midChildLoop, basicBlockMap));
        }
        return newLoop;
    }

    public ASMBasicBlock getPreHeader() {
        return preHeader;
    }

    public Set<ASMBasicBlock> getBodyBlocks() {
        return bodyBlocks;
    }

    public Set<ASMLoop> getChildLoopSet() {
        return childLoopSet;
    }
}
