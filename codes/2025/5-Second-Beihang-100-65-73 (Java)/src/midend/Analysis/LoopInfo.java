// 参考 https://zhuanlan.zhihu.com/p/20529760824

package midend.Analysis;

import frontend.ir.Value;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.Analysis.SCEV.Expr.SCEVConstant;
import midend.Analysis.SCEV.Expr.SCEVExpr;
import midend.Analysis.SCEV.LoopCondition;
import midend.Analysis.SCEV.SCEVManager;

import java.util.*;

public class LoopInfo {
    private final int counter;
    private int loopDepth;
    private LoopInfo parentLoop;
    private final HashSet<LoopInfo> childLoop;
    private final BasicBlock loopHeader;   // 被回边指向的块，也即第一个条件块
    private BasicBlock preHeader;
    private final HashSet<BasicBlock> enterFroms;  // 直连header的外部块
    private final HashSet<BasicBlock> exiting;
    private final HashSet<BasicBlock> exitTargets;
    private final HashSet<BasicBlock> bodyBlocks; // only this level
    private final HashSet<BasicBlock> latchBlocks;
    public LoopCondition loopCondition;
    public ArrayList<Instruction> loopContents;
    public LoopInfo tailLoop;
    public List<BasicBlock> topoSort; // 块集合和bodyBlocks完全重合

    public LoopInfo(int loopDepth, BasicBlock loopHeader, LoopInfo parentLoop) {
        this.counter = LoopAnalysis.getAndAddLoopCounter();
        this.loopDepth = loopDepth;
        this.parentLoop = parentLoop;
        this.childLoop = new HashSet<>();
        this.loopHeader = loopHeader;
        this.enterFroms = new HashSet<>();
        this.exiting = new HashSet<>();
        this.exitTargets = new HashSet<>();
        this.bodyBlocks = new HashSet<>();
        this.latchBlocks = new HashSet<>();
        this.loopCondition = null;
        this.loopContents = null;
        this.tailLoop = null;
        this.topoSort = new ArrayList<>();
    }

    @Override
    public String toString() {
        return toString(0);
    }

    private String toString(int indentLevel) {
        StringBuilder sb = new StringBuilder();
        String indent = "  ".repeat(indentLevel); // 每级缩进两个空格

        sb.append(indent).append("LoopInfo {\n");

        sb.append(indent).append("  counter: ").append(counter).append("\n");
        sb.append(indent).append("  loopDepth: ").append(loopDepth).append("\n");
        sb.append(indent).append("  parentLoop: ")
                .append(parentLoop == null ? "null" : parentLoop.getCounter())
                .append("\n");

        sb.append(indent).append("  loopHeader: ").append(loopHeader).append("\n");
        for (BasicBlock exit : exitTargets) {
            sb.append(indent).append("  loopHeaderDomedExit?: ").append(loopHeader.getDomSet().contains(exit)).append("\n");
        }
        sb.append(indent).append("  preHeader: ").append(preHeader).append("\n");
        sb.append(indent).append("  enterFroms: ").append(enterFroms).append("\n");
        sb.append(indent).append("  exiting: ").append(exiting).append("\n");
        sb.append(indent).append("  exitTargets: ").append(exitTargets).append("\n");
        sb.append(indent).append("  bodyBlocks: ").append(bodyBlocks).append("\n");
        sb.append(indent).append("  latchBlocks: ").append(latchBlocks).append("\n");

        if (!childLoop.isEmpty()) {
            sb.append(indent).append("  childLoops:\n");
            for (LoopInfo loopInfo : childLoop) {
                sb.append(loopInfo.toString(indentLevel + 2)).append("\n");
            }
        } else {
            sb.append(indent).append("  childLoops: []\n");
        }

        sb.append(indent).append("}");
        return sb.toString();
    }

    public void setLoopCondition(LoopCondition loopCondition) {
        this.loopCondition = loopCondition;
    }

    public HashSet<BasicBlock> getLatchBlocks() {
        return latchBlocks;
    }

    public BasicBlock getOnlyLatch() {
        if (latchBlocks.size() != 1) {
            throw new RuntimeException("LoopInfo.getOnlyLatch: latchBlocks size is not 1");
        }
        return latchBlocks.iterator().next();
    }

    public BasicBlock getExitTarget() {
        return exitTargets.iterator().next();
    }

    public HashSet<BasicBlock> getExiting() {
        return exiting;
    }

    public HashSet<BasicBlock> getExitTargets() {
        return exitTargets;
    }

    public int getCounter() {
        return counter;
    }

    public int getLoopDepth() {
        return loopDepth;
    }

    public void setLoopDepth(int loopDepth) {
        this.loopDepth = loopDepth;
    }

    public HashSet<BasicBlock> getEnterFroms() {
        return enterFroms;
    }

    public void setPreHeader(BasicBlock preHeader) {
        this.preHeader = preHeader;
    }

    public HashSet<LoopInfo> getChildLoop() {
        return childLoop;
    }

    public void addLatchBlocks(Set<BasicBlock> block) {
        latchBlocks.addAll(block);
    }

    public void addLatchBlock(BasicBlock block) {
        latchBlocks.add(block);
    }

    public void addBodyBlock(BasicBlock block) {
        bodyBlocks.add(block);
    }

    public BasicBlock getLoopHeader() {
        return loopHeader;
    }

    public LoopCondition getLoopCondition() {
        return loopCondition;
    }


    public LoopInfo getParentLoop() {
        return parentLoop;
    }

    public BasicBlock getPreHeader() {
        return preHeader;
    }

    public void setParentLoop(LoopInfo parentLoop) {
        this.parentLoop = parentLoop;
        if (parentLoop != null) {
            this.loopDepth = parentLoop.getLoopDepth() + 1;
        } else {
            this.loopDepth = 1;
        }
    }

    public void addChildLoop(LoopInfo childLoop) {
        this.childLoop.add(childLoop);
        childLoop.setParentLoop(this);
    }

    public LoopInfo getAncientLoop() {
        if (parentLoop == null) {
            return this;
        } else {
            return parentLoop.getAncientLoop();
        }
    }

    public boolean isParentOfThis(LoopInfo loop) {
        if (this.getParentLoop() == loop) {
            return true;
        } else if (this.getParentLoop() != null) {
            return this.getParentLoop().isParentOfThis(loop);
        }
        return false;
    }

    public Function getFunction() {
        return loopHeader.getParentFunc();
    }

    public HashSet<BasicBlock> getBodyBlocks() {
        return bodyBlocks;
    }

    public void addEnterFrom(BasicBlock block) {
        enterFroms.add(block);
    }

    public void addExiting(BasicBlock block) {
        exiting.add(block);
    }

    public void addExitTarget(BasicBlock block) {
        exitTargets.add(block);
    }

    public void getAllBlocks(HashSet<BasicBlock> blocks) {
        blocks.addAll(bodyBlocks);
        for (LoopInfo childLoop : childLoop) {
            childLoop.getAllBlocks(blocks);
        }
    }

    public int getInstrNum() {
        int instrNum = 0;
        HashSet<BasicBlock> blocks = new HashSet<>();
        getAllBlocks(blocks);
        for (BasicBlock block : blocks) {
            instrNum += block.getInstrNum();
        }
        return instrNum;
    }

    public boolean isSimpleLoop() {
        if (this.getExiting().size() != 1) {
            return false;
        }
        return this.getExiting().iterator().next() == this.getLoopHeader();
    }

    public boolean isConditionSimple() {
        return this.loopCondition != null && loopCondition.status &&
                loopCondition.tripCountExpr != SCEVManager.getSCEVCouldNotCompute()
                && !(loopCondition.bound instanceof Instruction ins && ins.getParentBB() == loopHeader);
    }

    /**
     * 循环变量初值可算且是常数返回初值，否则返回 null
     */
    public IntConst getLoopVarBeginningConst() {
        if (!isConditionSimple()) {
            throw new RuntimeException("条件必须是可算的");
        }

        SCEVExpr firstExpr = this.loopCondition.loopVarExpr.operands.getFirst();
        if (firstExpr instanceof SCEVConstant constant) {
            return new IntConst(constant.value);
        } else {
            return null;
        }
    }

    /**
     * 步长可算且是常数返回步长，否则返回 null
     */
    public IntConst getLoopVarStepConst() {
        if (!isConditionSimple() && this.loopCondition.loopVarExpr.operands.size() == 2) {
            throw new RuntimeException("条件必须是可算的");
        }

        SCEVExpr firstExpr = this.loopCondition.loopVarExpr.operands.getLast();
        if (firstExpr instanceof SCEVConstant constant) {
            return new IntConst(constant.value);
        } else {
            return null;
        }
    }

    public boolean isControlFlowSimple() {
        if (!this.getChildLoop().isEmpty()) {
            return false;
        }
        HashSet<BasicBlock> blocks = new HashSet<>();
        getAllBlocks(blocks);
        for (BasicBlock block : blocks) {
            if (block == loopHeader) {
                continue;
            }
            if (block.getSucs().size() > 1) {
                return false;
            }
        }
        return true;
    }

    public ArrayList<BasicBlock> getAllBlocksSorted() {
        ArrayList<BasicBlock> sortedBBs = new ArrayList<>();
        HashSet<BasicBlock> suc = new HashSet<>();
        suc.add(loopHeader);
        HashSet<BasicBlock> visited = new HashSet<>();
        while (!suc.isEmpty()) {
            BasicBlock bb = suc.iterator().next();
            suc.remove(bb);
            if (!visited.contains(bb) && !exitTargets.contains(bb)) {
                visited.add(bb);
            } else {
                continue;
            }
            sortedBBs.add(bb);
            suc.addAll(bb.getSucs());
        }
        return sortedBBs;
    }

    public Set<Instruction> usedInLCSSA() {
        Set<Instruction> usedInLCSSA = new HashSet<>();
        for (BasicBlock bb : getExitTargets()) {
            for (Instruction instr : bb.getInstrList()) {
                if (instr instanceof PhiInstr phi) {
                    if (!phi.isLCSSA()) {
                        continue;
                    }
                    for (Map.Entry<BasicBlock, Value> entry : phi.getOperandMap().entrySet()) {
                        if (exiting.contains(entry.getKey())) {
                            usedInLCSSA.add((Instruction) entry.getValue());
                        }
                    }
                } else {
                    break;
                }
            }
        }
        return usedInLCSSA;
    }

    /**
     * 删除之后CFG处于无效状态，只能删除单出口循环，即让preheader直接跳出口
     * !!! 在删除循环时，需要先解除对循环的LCSSA的使用
     */
    public void deleteCurrentLoop() {
        if (!childLoop.isEmpty()) {
            throw new RuntimeException("删除循环前，需要先删除子循环");
        }
        Set<Instruction> usedInLCSSA = usedInLCSSA();
        if (usedInLCSSA.isEmpty()) {
            if (loopDepth == 1) {
                this.getFunction().getAncientLoopInfo().remove(this);
            } else {
                this.getParentLoop().getChildLoop().remove(this);
            }
            for (BasicBlock block : bodyBlocks) {
                block.setLoopBelonged(null);
            }
            preHeader.replaceJumpTarget(loopHeader, exitTargets.iterator().next());
            for (Instruction instr : exitTargets.iterator().next().getInstrList()) {
                if (instr instanceof PhiInstr phi) {
                    phi.modifyUse(exiting.iterator().next(), preHeader);
                    exiting.iterator().next().getUserList().remove(phi);
                    preHeader.getUserList().add(phi);
                } else {
                    break;
                }
            }

            for (BasicBlock block : bodyBlocks) {
                block.removeFromListClear();
            }


        } else {
            throw new RuntimeException("删除循环后，循环中的指令仍被使用");
        }
    }

    /**
     * 只删除循环信息
     */
    public void deleteCurrentLoopLite() {
        if (loopDepth == 1) {
            this.getFunction().getAncientLoopInfo().remove(this);
            if (!childLoop.isEmpty()) {
                for (LoopInfo child : childLoop) {
                    child.setParentLoop(null);
                    this.getFunction().getAncientLoopInfo().add(child);
                }
            }
            for (BasicBlock bb : getBodyBlocks()) {
                bb.setLoopBelonged(null);
            }
        } else {
            this.getParentLoop().getChildLoop().remove(this);
            if (!childLoop.isEmpty()) {
                for (LoopInfo child : childLoop) {
                    child.setParentLoop(this.getParentLoop());
                    this.getParentLoop().getChildLoop().add(child);
                }
            }
            for (BasicBlock bb : getBodyBlocks()) {
                bb.setLoopBelonged(this.getParentLoop());
            }
        }
    }

    public LoopInfo getTailLoop() {
        return tailLoop;
    }

    public void addBlk2TopoSort(BasicBlock b) {
        if (bodyBlocks.contains(b)) {
            topoSort.add(b);
        }
    }

    public List<BasicBlock> getTopoSort() {
        return topoSort;
    }
}
