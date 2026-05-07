package mid.Optimizer.Loop;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Instruction.Cmp;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Instruction.Load;
import mid.IntermediatePresentation.Instruction.Phi;
import mid.IntermediatePresentation.User;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;
import mid.Optimizer.Optimizer;

import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;

public class Loop extends User {
    protected final BasicBlock header;
    protected final HashSet<BasicBlock> latchs = new HashSet<>();
    protected HashSet<BasicBlock> blocksInLoop = new HashSet<>();

    protected BasicBlock preheader;
    protected final HashSet<BasicBlock> exits = new HashSet<>();

    protected final HashSet<BasicBlock> exitings = new HashSet<>();

    protected HashMap<Value, ScEvValue> inds = new HashMap<>();

    protected HashMap<Value, Value> indWithPhi = new HashMap<>();

    public Loop(BasicBlock header) {
        super("LOOP", ValueType.NULL);
        this.header = header;
    }

    public HashMap<Value, ScEvValue> getInds() {
        return inds;
    }

    public HashMap<Value, Value> getIndWithPhi() {
        return indWithPhi;
    }

    public HashSet<BasicBlock> getBlocksInLoop() {
        return blocksInLoop;
    }

    public void addLatch(BasicBlock latch) {
        latchs.add(latch);
        /*
            这里是有问题的，需要确保就是循环header一定支配所有循环内的块
            比如  a->a,a->b,b->...->a，也就是a是一个子循环，那它的blocksInLoop只有a自己
            虽然a支配b而b也可达a，但是b并不是循环内的块，因为a->b->a经过了latch（a）
         */
        blocksInLoop.addAll(Optimizer.instance().getCFG().mayPassingBy(header, latch));
        blocksInLoop.retainAll(Optimizer.instance().getDominAnalyzer().getDominees(header));
        blocksInLoop.add(latch);
    }

    public void reAnalyzeBlocksInside() {
        blocksInLoop.clear();
        for (BasicBlock latch : latchs) {
            blocksInLoop.addAll(Optimizer.instance().getCFG().mayPassingBy(header, latch));
        }
        blocksInLoop.retainAll(Optimizer.instance().getDominAnalyzer().getDominees(header));
        blocksInLoop.addAll(latchs);
    }

    public BasicBlock getHeader() {
        return header;
    }

    public void setPreheader(BasicBlock preheader) {
        this.preheader = preheader;
    }

    public void setLatch(BasicBlock latch) {
        latchs.clear();
        addLatch(latch);
    }

    public BasicBlock getLatch() {
        for (BasicBlock latch : latchs) {
            return latch;
        }
        return null;
    }

    public void addExit(BasicBlock exit) {
        exits.add(exit);
    }

    public void addExiting(BasicBlock exiting) {
        exitings.add(exiting);
    }

    public HashSet<BasicBlock> getExitings() {
        return exitings;
    }

    public HashSet<BasicBlock> getLatchs() {
        return latchs;
    }

    public BasicBlock getPreheader() {
        return preheader;
    }

    public HashSet<BasicBlock> getExits() {
        return exits;
    }

    public boolean isConstant(Value v) {
        return !(v instanceof Load) && ((v instanceof ConstNumber) ||
                (v instanceof Instruction i) && !blocksInLoop.contains(i.getBlock()));
    }

    public void addInd(Phi phi, Value v, ScEvValue phiScev, ScEvValue vScev) {
        inds.put(phi, phiScev);
        inds.put(v, vScev);
        indWithPhi.put(phi, v);
        indWithPhi.put(v, phi);
    }

    public void removeInd(Value v) {
        inds.remove(v);
        indWithPhi.remove(v);
        for (Value u : new HashSet<>(indWithPhi.keySet())) {
            if (indWithPhi.get(u).equals(v)) {
                indWithPhi.remove(u);
            }
        }
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        if (preheader != null) {
            sb.append("preheader:").append(getPreheader().getReg()).append("\n");
        }
        sb.append("header:").append(getHeader().getReg()).append("\n");
        for (BasicBlock latch : getLatchs()) {
            sb.append("latch:").append(latch.getReg()).append("\n");
        }
        for (BasicBlock exit : getExits()) {
            sb.append("exit:").append(exit.getReg()).append("\n");
        }
        for (BasicBlock block : blocksInLoop) {
            sb.append("block:").append(block.getReg()).append("\n");
        }
        sb.append("\n");
        return sb.toString();
    }

}
