package Pass.IR.Utils;

import IR.Value.BasicBlock;
import IR.Value.Function;
import IR.Value.Instructions.CmpInst;
import IR.Value.Value;

import java.util.*;

public class IRLoop {
    private final BasicBlock header;
    private final ArrayList<IRLoop> subLoops = new ArrayList<>();
    private final ArrayList<BasicBlock> bbs = new ArrayList<>();
    private final HashSet<BasicBlock> exitingBlocks = new HashSet<>();
    // 循环退出后第一个到达的block
    private final HashSet<BasicBlock> exitBlocks = new HashSet<>();
    // 跳转到循环头的块
    private final ArrayList<BasicBlock> latchBlocks = new ArrayList<>();
    private IRLoop parentLoop = null;
    //  IndVar Info
    private Value itVar = null;
    private Value itEnd = null;
    private Value itInit = null;
    private Value itAlu = null;
    private Value itStep = null;
    private CmpInst headBrCond = null;
    private boolean setIndVar = false;
    private int times;
    //  loopFold info
    private boolean canLoopFold = false;
    private Value phiEnterValue;

    public IRLoop(BasicBlock header) {
        this.header = header;
        bbs.add(header);
    }

    public ArrayList<BasicBlock> getBbs(){
        return bbs;
    }

    public void addExitingBlock(BasicBlock bb) {
        exitingBlocks.add(bb);
    }

    public void addExitBlock(BasicBlock bb) {
        exitBlocks.add(bb);
    }

    public HashSet<BasicBlock> getExitBlocks(){
        return exitBlocks;
    }

    public void addLatchBlock(BasicBlock latchBb){
        latchBlocks.add(latchBb);
    }

    public BasicBlock getHead() {
        return header;
    }

    public boolean hasParent() {
        return parentLoop != null;
    }

    public IRLoop getParentLoop() {
        return parentLoop;
    }

    public void setParentLoop(IRLoop parentLoop) {
        this.parentLoop = parentLoop;
    }

    public void addSubLoop(IRLoop subLoop){
        subLoops.add(subLoop);
    }

    public void reverseBlock1() {
        Collections.reverse(bbs);
        BasicBlock bb = bbs.get(bbs.size() - 1);
        bbs.add(0, bb);
        bbs.remove(bbs.size() - 1);
    }

    public ArrayList<IRLoop> getSubLoops() {
        return subLoops;
    }

    public void addBlock(BasicBlock bb) {
        bbs.add(bb);
    }

    public int getLoopDepth() {
        int depth = 0;
        IRLoop now = this;
        while (now != null) {
            depth++;
            now = now.parentLoop;
        }
        return depth;
    }

    //  simpleLoop满足以下几个条件：
    //  1. 2个前驱（latch-head和正常运行-head）
    //  2. latch只有一个
    //  3. exiting只有一个
    //  4. exit只有一个
    public boolean isSimpleLoop(){
        if(header.getPreBlocks().size() != 2){
            return false;
        }
        if(latchBlocks.size() != 1){
            return false;
        }
        if(exitingBlocks.size() != 1){
            return false;
        }
        for(BasicBlock exitingBlock : exitingBlocks){
            if(exitingBlock != header){
                return false;
            }
        }
        return exitBlocks.size() == 1;
    }

    public ArrayList<BasicBlock> getLatchBlocks(){
        return latchBlocks;
    }

    public HashSet<BasicBlock> getExitingBlocks(){
        return exitingBlocks;
    }

    public void setIndInfo(Value itVar, Value itEnd, Value itInit, Value itAlu, Value itStep, CmpInst headBrCond){
        this.itVar = itVar;
        this.itEnd = itEnd;
        this.itInit = itInit;
        this.itAlu = itAlu;
        this.itStep = itStep;
        this.headBrCond = headBrCond;
        this.setIndVar = true;
    }

    public boolean isSetIndVar(){
        return setIndVar;
    }

    public void removeBb(BasicBlock bb){
        bbs.remove(bb);
    }

    public Value getItVar() {
        return itVar;
    }

    public Value getItEnd() {
        return itEnd;
    }

    public Value getItInit() {
        return itInit;
    }

    public Value getItAlu() {
        return itAlu;
    }

    public Value getItStep() {
        return itStep;
    }

    public CmpInst getHeadBrCond() {
        return headBrCond;
    }

    public void setItTimes(int times){
        this.times = times;
    }

    public int getItTimes(){
        return times;
    }
}
