package frontend.ir.structure;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.memop.AllocaInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.instr.terminator.Terminator;
import midend.Analysis.LoopInfo;
import midend.Analysis.LoopSimplify;
import util.CustomList;

import java.util.*;

public class BasicBlock extends Value {
    private final int index;
    private final CustomList instrList;
    private Function parentFunc;
    private boolean closed;   // 为真说明已经加入了 Terminator，构造阶段不再欢迎新指令。
    private final List<BasicBlock> pres;
    private final List<BasicBlock> sucs;
    private final Set<BasicBlock> domSet;   // 被 this 支配的基本块集合
    private final Set<BasicBlock> iDomSet;  // 被 this 直接支配的基本块集合
    private BasicBlock idom; // this 的立即支配者
    private int domLV;
    private final HashSet<BasicBlock> dominanceFrontier;   // this 的支配边界（刚好不被 this 支配的基本块集合）
    private LoopInfo loopBelonged;
    public HashSet<LoopInfo> isExitOflLoop;
    public BasicBlock sisterLoopHeader;
    public boolean isElder;

    public BasicBlock(int index) {
        super(null);
        this.index = index;
        this.instrList = new CustomList(this);
        this.parentFunc = null;
        this.closed = false;
        this.pres = new ArrayList<>();
        this.sucs = new ArrayList<>();
        this.domSet = new HashSet<>();
        this.iDomSet = new HashSet<>();
        this.dominanceFrontier = new HashSet<>();
        this.idom = null;
        this.domLV = 0;
        this.isExitOflLoop = new HashSet<>();
        this.sisterLoopHeader = null;
        this.isElder = false;
    }

    public void initCFGInfo() {
        this.domSet.clear();
        this.iDomSet.clear();
        this.dominanceFrontier.clear();
        this.idom = null;
        this.domLV = 0;
    }

    public void initLoopInfo() {
        this.loopBelonged = null;
        this.isExitOflLoop.clear();
    }

    public void setLoopBelonged(LoopInfo parentLoop) {
        this.loopBelonged = parentLoop;
    }

    public LoopInfo getLoopBelonged() {
        return loopBelonged;
    }

    public int getIndex() {
        return index;
    }

    public boolean isContainsInLoop(LoopInfo loopInfo) {
        if (this.getLoopBelonged() == null) {
            return false;
        } else {
            LoopInfo ancestor = this.getLoopBelonged();
            while (ancestor != null) {
                if (ancestor == loopInfo) {
                    return true;
                }
                ancestor = ancestor.getParentLoop();
            }
        }
        return false;
    }

    public void replaceOnlyPhi(BasicBlock newBB) {
        for (Value user : new ArrayList<>(this.getUserList())) {
            if (user instanceof PhiInstr) {
                user.modifyUse(this, newBB);
                newBB.getUserList().add(user);
                this.getUserList().remove(user);
            }
        }
    }

    public boolean isEntry() {
        return this.getParentFunc().getFirstBlk() == this;
    }

    public void addInstruction(Instruction instruction) {
        this.instrList.addToTail(instruction);
        instruction.setParentBB(this);
    }

    @Override
    public void insertAfter(CustomList.Node node) {
        super.insertAfter(node);
        if (node instanceof BasicBlock blk) {
            this.parentFunc = blk.parentFunc;
        } else {
            throw new RuntimeException("bb can only be inserted into bb list");
        }
    }

    @Override
    public void insertBefore(CustomList.Node node) {
        super.insertBefore(node);
        if (node instanceof BasicBlock blk) {
            this.parentFunc = blk.parentFunc;
        } else {
            throw new RuntimeException("bb can only be inserted into bb list");
        }
    }

    public void setParentFunc(Function parentFunc) {
        this.parentFunc = parentFunc;
    }

    public Function getParentFunc() {
        return parentFunc;
    }

    /**
     * 获取从自己开始到 node 的连续 Node 列表，如果找不到这样的列表抛异常
     * 返回列表包括两端 (this & node)
     */
    public List<BasicBlock> getSubListTo(BasicBlock node) {
        BasicBlock current = this;
        List<BasicBlock> subList = new ArrayList<>();
        while (current != null && current != node) {
            subList.add(current);
            current = (BasicBlock) current.getNext();
        }
        if (current == node) {
            subList.add(node);
            return subList;
        } else {
            throw new RuntimeException("找不到要求的列表");
        }
    }

    public List<Instruction> getInstrList() {
        List<Instruction> insList = new ArrayList<>();
        for (CustomList.Node node : instrList) {
            if (!(node instanceof Instruction instruction)) {
                throw new RuntimeException("指令列表里只能有指令");
            }
            insList.add(instruction);
        }
        return insList;
    }

    public boolean isEmpty() {
        return this.instrList.isEmpty();
    }

    public Instruction getFirstInstr() {
        return (Instruction) this.instrList.getHead();
    }

    public Instruction getLastInstr() {
        return (Instruction) this.instrList.getTail();
    }

    public List<AllocaInstr> popAllAlloca() {
        List<AllocaInstr> list = new ArrayList<>();
        CustomList.Node node = instrList.getHead();
        while (node != null) {
            CustomList.Node next = node.getNext();
            if (node instanceof AllocaInstr allocaInstr) {
                allocaInstr.pureRemoveFromList();
                list.add(allocaInstr);
            }
            node = next;
        }
        return list;
    }

    public void insertInsListToHead(List<Instruction> newInsList) {
        for (int i = newInsList.size() - 1; i >= 0; i--) {
            insertInsToHead(newInsList.get(i));
        }
    }

    public void insertInsToHead(Instruction newIns) {
        this.instrList.addToHead(newIns);
        newIns.setParentBB(this);
    }

    public void insertInsBefore(Instruction toInsert, Instruction pos) {
        toInsert.insertBefore(pos);
        toInsert.setParentBB(this);
    }

    public void close() {
        this.closed = true;
    }

    public void open() {
        this.closed = false;
    }

    public boolean isNotClosed() {
        return !closed;
    }

    public void addPre(BasicBlock pre) {
        this.pres.add(pre);
    }

    public void addSuc(BasicBlock suc) {
        this.sucs.add(suc);
    }

    public boolean delPre(BasicBlock toDel) {
        return this.pres.remove(toDel);
    }

    public boolean delSuc(BasicBlock toDel) {
        return this.sucs.remove(toDel);
    }

    public List<BasicBlock> getPres() {
        return pres;
    }

    public List<BasicBlock> getSucs() {
        return sucs;
    }

    public Set<BasicBlock> getDomSet() {
        return domSet;
    }

    /**
     * 初始化支配集合，自己肯定支配自己
     */
    public void initDomSet() {
        this.domSet.clear();
        this.domSet.add(this);
    }

    public Set<BasicBlock> getIDomSet() {
        return iDomSet;
    }

    public void setIDom(BasicBlock idom) {
        this.idom = idom;
//        this.domLV = idom.domLV + 1;
    }

    public void setDomLV(int depth) {
        this.domLV = depth;
    }

    public int getDomLV() {
        return domLV;
    }

    public BasicBlock getIDom() {
        return idom;
    }

    public void initIDomSet() {
        this.iDomSet.clear();
    }

    public HashSet<BasicBlock> getDominanceFrontier() {
        return dominanceFrontier;
    }

    @Override
    public void removeFromList() {
        for (CustomList.Node node : this.instrList) {   // 块内相互使用会导致无法删除
            for (Value usedVal : ((Value) node).getUsedList()) {
                usedVal.getUserList().remove(node);
            }
        }
        for (CustomList.Node node : this.instrList) {
            if (node instanceof Instruction ins && this.getUsedList().contains(ins)) {
                this.getUsedList().remove(ins);
                ins.getUserList().remove(this);
            }
            assert node instanceof Instruction;
            ((Instruction) node).forceRemoveFromList();
        }

        for (Value user : new ArrayList<>(this.getUserList())) {
            if (user instanceof PhiInstr phi) {
                phi.delOpPair(this);
                phi.simplify();
            }
        }
        super.removeFromList();
    }

    public void removeFromListClear() {
        for (CustomList.Node node : this.instrList) {
            if (node instanceof Instruction ins) {
                ins.forceRemoveFromList();
                ins.getUserList().clear();
            }
        }
        this.getUsedList().clear();
        super.removeFromList();
    }

    public void pureRemoveFromList() {
        super.removeFromList();
    }

    @Override
    public String toString() {
        return this.value2string();
    }

    @Override
    public String value2string() {
        if (parentFunc == null) {
            return "unknownFunc_blk_" + index;
        } else {
            return parentFunc.getName() + "_blk_" + index;
        }
    }

    public int getLoopDepth() {
        if (loopBelonged == null) {
            return 0;
        }
        return loopBelonged.getLoopDepth();
    }

    public void replaceJumpTarget(BasicBlock original, BasicBlock newTarget) {
        if (instrList.getTail() instanceof JumpInstr j) {
            j.modifyUse(original, newTarget);
            original.getUserList().remove(j);
            newTarget.getUserList().add(j);
        } else if (instrList.getTail() instanceof BranchInstr b) {
            b.modifyUse(original, newTarget);
            original.getUserList().remove(b);
            newTarget.getUserList().add(b);
        } else {
            throw new RuntimeException("跳转指令不存在");
        }
    }

    public ArrayList<PhiInstr> getPhiInstrs() {
        ArrayList<PhiInstr> phiInstrs = new ArrayList<>();
        for (CustomList.Node node : instrList) {
            if (node instanceof PhiInstr phi) {
                phiInstrs.add(phi);
            } else {
                break;
            }
        }
        return phiInstrs;
    }

    public int getInstrNum() {
        return instrList.getSize();
    }

    public boolean dominates(BasicBlock maySub) {
        return this.getDomSet().contains(maySub);
    }

    public List<Instruction> getExecutableInstrList() {
        List<Instruction> executable = new ArrayList<>();
        for (Instruction instr : getInstrList()) {
            if (instr instanceof PhiInstr) continue;
            else if (instr instanceof Terminator) break;
            executable.add(instr);
        }
        return executable;
    }
}
