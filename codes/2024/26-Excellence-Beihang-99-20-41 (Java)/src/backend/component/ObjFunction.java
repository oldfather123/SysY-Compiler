package backend.component;

import backend.instruction.ObjBinary;
import backend.instruction.ObjMove;
import backend.operand.ObjOperand;
import backend.operand.ObjReg;
import ir.value.Argument;
import utils.RefCell;

import java.util.*;

public class ObjFunction {
    private String name;
    private LinkedList<Argument> args;
    private LinkedList<ObjBlock> blocks;
    private HashMap<String,ObjBlock> blockMap;
    private HashSet<ObjReg> regs;//使用的虚拟寄存器，仅包含ObjVirReg和ObjFVirReg
    private int allocaSize = 0, raSize = 0, argsSize = 0;//分别对应alloca分配的空间、存储ra的空间、最多参数的空间
    private int allocaStart = 0;//从哪里开始/继续分配alloca

    public void resetArgsLoadTarget(int stackSize) {
        for (var instr : getFirstBlock().getInstrs()) {
            if (instr instanceof ObjMove move) {
                move.appendOffsetIfMarked(stackSize);
            }
        }
    }

    private final RefCell<ObjBinary> allocStack = new RefCell<>(null);
    private final Map<ObjBlock, ObjBinary> freeStackBlock = new HashMap<>();

    public void setAllocStack(ObjBinary op) {
        allocStack.set(op);
    }

    public ObjBinary getAllocStack() {
        return allocStack.get();
    }

    public void addFreeStack(ObjBinary op, ObjBlock block) {
        freeStackBlock.put(block, op);
    }

    public ObjBinary getFreeStack(ObjBlock block) {
        return freeStackBlock.get(block);
    }

    public void resetStackSize(ObjOperand plus, ObjOperand minus) {
        allocStack.get().setSrc2(minus);
        freeStackBlock.values().forEach(s -> s.setSrc2(plus));
    }

    public ObjFunction(String name) {
        this.name = name;
        blocks = new LinkedList<>();
        args = new LinkedList<>();
        regs = new HashSet<>();
        blockMap=new HashMap<>();
    }

    public ObjFunction(String name, LinkedList<Argument> args, LinkedList<ObjBlock> blocks) {
        this.name = name;
        this.args = args;
        this.blocks = blocks;
        blockMap=new HashMap<>();
    }

    public void addUsedVirReg(ObjReg objVirReg) {
        regs.add(objVirReg);
    }

    public String getName() {
        return name;
    }

    public LinkedList<ObjBlock> getBlocks() {
        return blocks;
    }

    public HashMap<String, ObjBlock> getBlockMap() {
        return blockMap;
    }

    public LinkedList<Argument> getArgs() {
        return args;
    }

    public HashSet<ObjReg> getOperands() {
        return regs;
    }

    public void setOperands(HashSet<ObjReg> operands) {
        this.regs = operands;
    }

    public void addObjBlock(ObjBlock objBlock) {
        blocks.addLast(objBlock);
    }

    public void addFirstBlock(ObjBlock objBlock) {
        blocks.addFirst(objBlock);
    }

    public void addArg(Argument argument) {
        args.add(argument);
    }

    public void addReg(ObjReg reg) {
        regs.add(reg);
    }

    public void addAllocaSize(int size) {
        allocaSize += size;
    }

    public int getAllocaSize() {
        return allocaSize;
    }

    public void setRaSize() {
        raSize = 8;
    }

    public int getRaSize() {
        return raSize;
    }

    public void setArgsSize(int size) {
        if (size > argsSize) {
            argsSize = size;
        }
        allocaStart = argsSize;
    }

    public int getArgsSize() {
        return argsSize;
    }

    public int getStackSize() {
        int size = raSize + argsSize + allocaSize;
        if (size % 8 != 0) {
            size += 4;
        }
        return size;
    }

    public int getAllocaStart() {
        return allocaStart;
    }

    public void addAllocaStart(int delta) {
        allocaStart += delta;
    }

    private final Set<ObjBlock> exits = new HashSet<>();

    public void setExits(ObjBlock b) {
        if (b != null)
            this.exits.add(b);
    }
    public List<ObjBlock> getExits() {
        return new ArrayList<>(exits);
    }

    @Override
    public String toString() {
        StringBuilder string = new StringBuilder(name + ":\n");
        for (ObjBlock block : blocks) {
            string.append(block.toString());
        }
        return string.toString();
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj == null || !(obj instanceof ObjFunction)) {
            return false;
        }
        return ((ObjFunction) obj).name.equals(this.name);
    }

    @Override
    public int hashCode() {
        return Objects.hash(name);
    }

    public ObjBlock getFirstBlock() {
        return blocks.getFirst();
    }
}
