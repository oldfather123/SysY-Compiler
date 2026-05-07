package backend.itemStructure;

import Utils.CustomList;
import backend.regs.AsmReg;
import frontend.ir.Value;
import frontend.ir.structure.BasicBlock;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

public class AsmFunction {
    String name;
    private CustomList blocks = new CustomList();
    private HashSet<AsmReg> usedVirRegs = new HashSet<>();

    private int allocaSize = 0;
    private int raSize = 0;
    private int argsSize = 0;
    boolean isTail = false;
    public List<Value> iargs = new ArrayList<>();
    public List<Value> fargs = new ArrayList<>();

    public AsmFunction(String name) {
        this.name = name;
    }

    public void addBlock(AsmBlock block) {
        blocks.addToTail(block);
    }

    public void setArgsSize(int argsSize) {
        this.argsSize = Math.max(argsSize, this.argsSize);
    }

    public void setRaSize(int raSize) {
        this.raSize = raSize;
    }

    public void setIsTail(boolean isTail) {
        this.isTail = isTail;
    }

    public int getArgsSize() {
        return argsSize;
    }

    public int getAllocaSize() {
        return allocaSize;
    }

    public int addAllocaSize(int size) {
        return allocaSize += size;
    }

    public void addUsedVirReg(AsmReg reg) {
        usedVirRegs.add(reg);
    }

    public int getWholeSize() {
        int size = raSize + argsSize + allocaSize;
        return size;
    }

    public int getRaSize() {
        return raSize;
    }

    public String getName() {
        return name;
    }

    public CustomList getBlocks() {
        return blocks;
    }

    public AsmBlock getTailBlock() {
        return (AsmBlock) blocks.getTail();
    }

    public String toString() {
        //应该无法用到
        return null;
    }

    public void addIntAndFloatFunctions(List<Value> iargs, List<Value> fargs) {
        this.iargs.addAll(iargs);
        this.fargs.addAll(fargs);
    }

    public void addBlockToHead(AsmBlock block) {
        blocks.addToHead(block);
    }

    public CustomList.Node getHead() {
        return blocks.getHead();
    }

}
