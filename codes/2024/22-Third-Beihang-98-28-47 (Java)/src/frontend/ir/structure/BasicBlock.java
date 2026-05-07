package frontend.ir.structure;

import Utils.CustomList;
import debug.DEBUG;
import frontend.ir.DataType;
import frontend.ir.Use;
import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.memop.AllocaInstr;
import frontend.ir.instr.memop.MemoryOperation;
import frontend.ir.instr.otherop.PCInstr;
import frontend.ir.instr.otherop.cmp.Cmp;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.instr.terminator.ReturnInstr;
import midend.loop.BlockType;

import java.io.IOException;
import java.io.Writer;
import java.util.*;

public class BasicBlock extends Value {
    private CustomList instructions = new CustomList(this);
    private BasicBlock newTrue;
    private BasicBlock newFalse;
    private int labelCnt;
    private int loopDepth;
    private int domDepth;
    private boolean isRet;
    private boolean isEntering;
    private int blockType;
    private HashSet<BasicBlock> pres;
    private HashSet<BasicBlock> sucs;
    private HashSet<BasicBlock> doms;
    private HashSet<BasicBlock> loopDoms;
    private HashSet<BasicBlock> iDoms;
    private BasicBlock iDomor;
    private HashSet<BasicBlock> DF;
    private String tag = "";    // 用作一些特殊标记
    private final HashMap<BasicBlock, Value> conditions = new HashMap<>();    // 用来存储进入这个块所必须满足的一系列条件
    
    public BasicBlock(int loopDepth, int labelCnt) {
        super();
        isRet = false;
        isEntering = false;
        this.loopDepth = loopDepth;
        this.domDepth = 0;
        pres = new HashSet<>();
        sucs = new HashSet<>();
        doms = new HashSet<>();
        loopDoms = new HashSet<>();
        iDoms = new HashSet<>();
        DF = new HashSet<>();
        this.labelCnt = labelCnt;
        newTrue = newFalse = null;
        blockType = BlockType.OUTOFLOOP.getNum();
    }

    public void removeFromListWithUseRemain() {
        super.removeFromList();
        this.setNext(null);
        this.setPrev(null);
    }

    public boolean isBlockType(BlockType blockType) {
        return (this.blockType & blockType.getNum()) != 0;
    }

    public void setBlockType(BlockType blockType) {
        this.blockType = blockType.getNum() | this.blockType;
    }

    public void setNewTrue(BasicBlock newTrue) {
        this.newTrue = newTrue;
    }

    public void setNewFalse(BasicBlock newFalse) {
        this.newFalse = newFalse;
    }

    public BasicBlock getNewTrue() {
        return newTrue;
    }

    public BasicBlock getNewFalse() {
        return newFalse;
    }

    public void setInstructions(CustomList instructions) {
        this.instructions = instructions;
    }

    public void setRet(boolean ret) {
        isRet = ret;
    }

    public int getLoopDepth() {
        return loopDepth;
    }

    public void setLoopDepth(int loopDepth) {
        this.loopDepth = loopDepth;
    }
    public int getDomDepth() {
        return domDepth;
    }

    public void setDomDepth(int domDepth) {
        this.domDepth = domDepth;
    }

    public Instruction getEndInstr() {
        return (Instruction) instructions.getTail();
    }

    public void setDF(HashSet<BasicBlock> DF) {
        this.DF = DF;
    }

    public void setIDoms(HashSet<BasicBlock> iDoms) {
        this.iDoms = iDoms;
    }

    public void setPres(HashSet<BasicBlock> pres) {
        this.pres = pres;
    }

    public void setSucs(HashSet<BasicBlock> sucs) {
        this.sucs = sucs;
    }

    public void setDoms(HashSet<BasicBlock> doms) {
        this.doms = doms;
    }

    public void setLoopDoms(HashSet<BasicBlock> loopDoms) {
        this.loopDoms = loopDoms;
    }

    public HashSet<BasicBlock> getIDoms() {
        return iDoms;
    }

    public HashSet<BasicBlock> getDF() {
        return DF;
    }

    public HashSet<BasicBlock> getPres() {
        return pres;
    }

    public HashSet<BasicBlock> getSucs() {
        return sucs;
    }

    public HashSet<BasicBlock> getLoopDoms() {
        return loopDoms;
    }

    public HashSet<BasicBlock> getDoms() {
        return doms;
    }

    public void setLabelCnt(int labelCnt) {
        this.labelCnt = labelCnt;
    }

    public int getLabelCnt() {
        return labelCnt;
    }
    
    public List<MemoryOperation> popAllAlloca() {
        Instruction instr = (Instruction) instructions.getHead();
        ArrayList<MemoryOperation> allocaList = new ArrayList<>();
        while (instr != null) {
            Instruction tmp = (Instruction) instr.getNext();
            if (instr instanceof AllocaInstr) {
                allocaList.add((AllocaInstr) instr);
                instr.removeFromListWithUseRemain();
            }
            instr = tmp;
        }
        return allocaList;
    }
    
    public void reAddAllAlloca(List<MemoryOperation> allocaInstrList) {
        if (allocaInstrList == null) {
            throw new NullPointerException();
        }
        for (int i = allocaInstrList.size() - 1; i >= 0; i--) {
            this.addInstrToHead(allocaInstrList.get(i));
        }
    }

    public void addInstruction(Instruction instr) {
        //removeFromList销毁所有setUse
        instr.setParentBB(this);
        instructions.addToTail(instr);
        if (isRet) {
            instr.removeFromList();
            return;
        }
        if (instr instanceof ReturnInstr) {
            isRet = true;
        } else if (instr instanceof JumpInstr) {
            ((JumpInstr) instr).setRelation(this);
            isRet = true;
        } else if (instr instanceof BranchInstr) {
            ((BranchInstr) instr).setRelation(this);
            isRet = true;
        }
    }

    public void addInstrToHead(Instruction instr) {
        instr.setParentBB(this);
        instructions.addToHead(instr);
    }

    public void printIR(Writer writer) throws IOException {
        if (writer == null) {
            throw new NullPointerException();
        }

        for (CustomList.Node instructionNode : instructions) {
            Instruction instruction = (Instruction) instructionNode;
            writer.append("\t").append(instruction.print()).append("\n");
        }
    }
    
    @Override
    public Number getNumber() {
        throw new RuntimeException("基本块暂时没有值");
    }
    
    @Override
    public DataType getDataType() {
        throw new RuntimeException("基本块暂时没有数据类型");
    }
    
    @Override
    public String value2string() {
        if (DEBUG.debug1) {
            return "blk_" + labelCnt  + "_@" + ((Procedure) this.getParent().getOwner()).getParentFunc().toString();
        } else {
            return "blk_" + labelCnt;
        }
    }

    public CustomList getInstructions() {
        return instructions;
    }

    public void printHashset(HashSet<BasicBlock> hashSet, String s) {
        StringBuilder str = new StringBuilder(s);
        str.append(": ");
        for(BasicBlock b : hashSet) {
            str.append(b.value2string()).append(" ");
        }
        DEBUG.dbgPrint2(String.valueOf(str));
    }

    public void printDBG() {
        StringBuilder str;
        DEBUG.dbgPrint("blk: " + value2string());
        DEBUG.dbgPrint("used: ");
        Use use = this.getBeginUse();
        if (use == null) {
            DEBUG.dbgPrint1("chao???");
        } else {
           Instruction instruction =  use.getUser();
            BasicBlock block = instruction.getParentBB();
            DEBUG.dbgPrint1(block.value2string());
        }
        printHashset(sucs, "sucs");
        printHashset(pres, "pres");
        printHashset(doms, "doms");
        printHashset(iDoms, "iDoms");
        printHashset(DF, "DF");
        //printIDomor
        str = new StringBuilder("idomor");
        str.append(": ");
        if (iDomor == null) {
            str.append("null ");

        } else {
            str.append(iDomor.value2string()).append(" ");
        }
        DEBUG.dbgPrint2(String.valueOf(str));
        DEBUG.dbgPrint1("");
    }

    @Override
    public void removeFromList() {
        super.removeFromList();
        Instruction instr = (Instruction) instructions.getHead();
        while (instr != null) {
            instr.removeFromList();
            instr = (Instruction) instr.getNext();
        }
    }
    
    public void removeFromListWithInstrRemain() {
        super.removeFromList();
    }

    public PCInstr getPc() {
        assert instructions.getHead() instanceof PCInstr;
        return (PCInstr) instructions.getHead();
    }

    public BasicBlock clone4while(Procedure procedure, HashMap<Value, Value> old2new) {
        BasicBlock newBlk = new BasicBlock(loopDepth, procedure.getAndAddBlkIndex());
        Instruction instr = (Instruction) this.getInstructions().getHead();
        while (instr != null) {
            Instruction newIns = instr.cloneShell(procedure);
            newBlk.addInstruction(newIns);
            for (Value value : instr.getUseValueList()) {
                Value newValue = old2new.get(value);
                if (newValue != null) {
                    newIns.modifyUse(value, newValue);
                }
            }
            old2new.put(instr, newIns);
            instr = (Instruction) instr.getNext();
        }
        return newBlk;
    }

    public BasicBlock clone4while(BasicBlock newBlk, Procedure procedure) {
        HashMap<Value, Value> old2new = new HashMap<>();
        Instruction instr = (Instruction) this.getInstructions().getHead();
        while (instr != null) {
            Instruction newIns = instr.cloneShell(procedure);
            for (Value value : instr.getUseValueList()) {
                Value newValue = old2new.get(value);
                if (newValue != null) {
                    newIns.modifyUse(value, newValue);
                }
            }
            newBlk.addInstruction(newIns);
            old2new.put(instr, newIns);
            instr = (Instruction) instr.getNext();
        }
        return newBlk;
    }
    public void setiDomor(BasicBlock iDomor) {
        this.iDomor = iDomor;
    }

    public BasicBlock getiDomor() {
        return iDomor;
    }

    public void setEntering(boolean entering) {
        isEntering = entering;
    }

    public boolean isEntering() {
        return isEntering;
    }
    
    public String getTag() {
        return tag;
    }
    
    public void setTag(String tag) {
        this.tag = tag;
    }
    
    public void addCond(BasicBlock from, Value cond) {
        this.conditions.put(from, cond);
    }
    
    public HashMap<BasicBlock, Value> getConditions() {
        return conditions;
    }
}
