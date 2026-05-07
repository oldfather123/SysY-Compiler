package midend.analysis;

import midend.BasicBlock;
import midend.Value;
import midend.instr.Instruction;

import java.util.ArrayList;
import java.util.Collections;

public class Loop {
    private final BasicBlock header;
    private final ArrayList<BasicBlock> basicBlocks = new ArrayList<>();
    private Loop parentLoop = null;
    private final ArrayList<Loop> subLoops = new ArrayList<>();
    private final ArrayList<BasicBlock> exitBlock = new ArrayList<>();
    private Value itBegin = null;
    private Value itVar = null;
    private Value itEnd = null;
    private String op = null;
    private boolean useLess = false;
    private Value itAlu = null;
    private Value itChange = null;
    private Value itCmp = null;
    private boolean isIter = false;

    public Loop(BasicBlock header) {
        this.header = header;
        addBlock(header);
    }

    public void setLoopIterator(Value itBegin, Value itCmp, Value itVar, Value itEnd, String op, Value itAlu, Value itChange, boolean useLess) {
        this.itBegin = itBegin;
        this.itVar = itVar;
        this.itCmp = itCmp;
        this.itEnd = itEnd;
        this.op = op;
        this.itAlu = itAlu;
        this.itChange = itChange;
        this.useLess = useLess;
        this.isIter = !useLess;
    }

    public boolean isIter() {
        return this.isIter;
    }

    public Value getItCmp() {
        return this.itCmp;
    }

    public Value getItBegin() {
        return this.itBegin;
    }

    public Value getItVar() {
        return this.itVar;
    }

    public Value getItEnd() {
        return this.itEnd;
    }

    public Value getItAlu() {
        return this.itAlu;
    }

    public Value getItChange() {
        return this.itChange;
    }

    public String getOp() {
        return this.op;
    }

    public boolean isUseLess() {
        return this.useLess;
    }

    public void setUseLess() {
        this.useLess = true;
    }

    public BasicBlock getHeader() {
        return this.header;
    }

    public ArrayList<BasicBlock> getBasicBlocks() {
        return this.basicBlocks;
    }

    public void addBlock(BasicBlock block) {
        this.basicBlocks.add(block);
    }

    public void reverseBlock() {
        Collections.reverse(basicBlocks);
        BasicBlock bb = basicBlocks.get(basicBlocks.size() - 1);
        basicBlocks.add(0, bb);
        basicBlocks.remove(basicBlocks.size() - 1);
    }

    public int getLoopDepth() {
        int count = 0;
        Loop cur = this;
        while (cur != null) {
            count++;
            cur = cur.getParentLoop();
        }
        return count;
    }

    public ArrayList<BasicBlock> getExitBlock() {
        return this.exitBlock;
    }

    public void addExitBlock(BasicBlock block) {
        if (!this.exitBlock.contains(block)) {
            this.exitBlock.add(block);
        }
    }

    public ArrayList<Loop> getSubLoops() {
        return this.subLoops;
    }

    public void addSubLoop(Loop loop) {
        this.subLoops.add(loop);
    }

    public void reverseSubLoop() {
        Collections.reverse(this.subLoops);
    }

    public Loop getParentLoop() {
        return this.parentLoop;
    }

    public void setParentLoop(Loop loop) {
        this.parentLoop = loop;
    }
}

