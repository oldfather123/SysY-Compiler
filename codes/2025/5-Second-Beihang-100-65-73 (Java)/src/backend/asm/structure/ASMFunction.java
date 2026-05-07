package backend.asm.structure;

import backend.asm.register.phy.PhyReg;
import backend.asm.register.store.RegStore;
import backend.opt.ASMLoop;
import frontend.ir.structure.BasicBlock;
import midend.Analysis.LoopInfo;
import util.CustomList;

import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;

public class ASMFunction {
    public static final int RA_SIZE = 8;
    private final String name;
    private int allocatedSize = 0;
    private int reservedParamSize;
    private final CustomList basicBlockList;
    private final Set<PhyReg> usedRegs;
    private final Set<ASMFunction> calleeSet;
    private final boolean isLibFunc;
    private final boolean isTailRecursive; // 是否是尾递归函数
    private final boolean isParallelLoopBody;   // 是否是并行循环体函数
    // private ASMBasicBlock bstartBlock = null;
    private final Set<ASMLoop> ancientLoopInfo;

    public ASMFunction(String name, boolean isTailRecursive, boolean isParallelLoopBody) {
        this.name = name;
        this.basicBlockList = new CustomList(this);
        this.usedRegs = new LinkedHashSet<>();
        this.calleeSet = new LinkedHashSet<>();
        this.isLibFunc = false;
        this.isTailRecursive = isTailRecursive;
        this.isParallelLoopBody = isParallelLoopBody;
        this.ancientLoopInfo = new LinkedHashSet<>();
    }

    public ASMFunction(String name, boolean isLibFunc, RegStore regStore) {
        this.name = name;
        this.basicBlockList = new CustomList(this);
        this.usedRegs = new LinkedHashSet<>();
        this.calleeSet = new LinkedHashSet<>();
        this.isLibFunc = isLibFunc;
        this.isTailRecursive = false;//库函数不可能是尾递归
        this.isParallelLoopBody = false;
        this.ancientLoopInfo = new LinkedHashSet<>();

        if (isLibFunc) {
            this.usedRegs.addAll(regStore.getAllTempRegs());
        }
    }

    public void initAncientLoopInfo(Set<LoopInfo> midLoopInfoSet, Map<BasicBlock, ASMBasicBlock> basicBlockMap) {
        this.ancientLoopInfo.clear();
        for (LoopInfo loopInfo : midLoopInfoSet) {
            this.ancientLoopInfo.add(ASMLoop.initLoop(loopInfo, basicBlockMap));
        }
    }

    public Set<ASMLoop> getAncientLoopInfo() {
        return ancientLoopInfo;
    }

    public void addAllocatedSize(int delta) {
        this.allocatedSize += delta;
    }

    public void insertBlock2Tail(ASMBasicBlock basicBlock) {
        this.basicBlockList.addToTail(basicBlock);
    }

    public CustomList getBasicBlockList() {
        return basicBlockList;
    }

    public void setReservedParamSize(int reservedParamSize) {
        this.reservedParamSize = reservedParamSize;
    }

    public int getAllocatedSize() {
        return allocatedSize;
    }

    public int getReservedParamSize() {
        return reservedParamSize;
    }

    public int getStackSize() {
        return RA_SIZE + allocatedSize + reservedParamSize;
    }

    public boolean isMain() {
        return this.name.equals("main");
    }

    public boolean isLibFunc() {
        return this.isLibFunc;
    }

    public boolean isTailRecursive() {
        return this.isTailRecursive;
    }

    public ASMBasicBlock createBasicBlockAtHead() {
        String blkName = this.name + "_head_blk_" + this.basicBlockList.getSize();
        ASMBasicBlock newBlock = new ASMBasicBlock(this, 0, blkName);
        newBlock.removeFromList();
        this.basicBlockList.addToHead(newBlock);
        return newBlock;
    }
//    public ASMBasicBlock getBstartBlock() {
//        return bstartBlock;
//    }
//
//    public void setBstartBlock(ASMBasicBlock bstartBlock) {
//        this.bstartBlock = bstartBlock;
//    }

    public void addUsedReg(PhyReg register) {
        this.usedRegs.add(register);
    }

    public Set<PhyReg> getUsedRegs() {
        Set<PhyReg> allUsedRegs = new LinkedHashSet<>(this.usedRegs);
        for (ASMFunction callee : calleeSet) {
            if (callee != this) {
                allUsedRegs.addAll(callee.getUsedRegs());
            }
        }
        return allUsedRegs;
    }

    public void addCallee(ASMFunction newCallee) {
        this.calleeSet.add(newCallee);
    }

    public Set<ASMFunction> getCalleeSet() {
        return calleeSet;
    }

    @Override
    public String toString() {
        return this.name;
    }

    public boolean isParallelLoopBody() {
        return isParallelLoopBody;
    }
}
