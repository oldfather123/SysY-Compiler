package midend;

import midend.LLVMType.Type;
import midend.LLVMType.UndefinedType;
import midend.LLVMType.VoidType;
import midend.instr.Instruction;
import midend.instr.PhiInstr;

import java.util.ArrayList;
import java.util.LinkedList;

public class BasicBlock extends Value{
    private LinkedList<Instruction> instrList;
    private ArrayList<BasicBlock> preBlock = new ArrayList<>();
    private ArrayList<BasicBlock> nextBlock = new ArrayList<>();
    private ArrayList<BasicBlock> domBlock = new ArrayList<>();
    private BasicBlock parentBlock;
    private ArrayList<BasicBlock> childBlock = new ArrayList<>();

    private ArrayList<BasicBlock> DF;
    private Function parentFunc;
    private static int blockNum = 0;
    private int domDepth = 0;

    public BasicBlock(Type type, Function function) {
        super(type, "block" + String.valueOf(blockNum++));
        this.parentFunc = function;
        this.instrList = new LinkedList<>();
    }

    public int getLoopDepth() {
        return this.parentFunc.getLoopDepth(this);
    }

    public void setDomDepth(int depth) {
        this.domDepth = depth;
    }

    public int getDomDepth() {
        return this.domDepth;
    }

    public void setParentFunc(Function function) {
        this.parentFunc = function;
    }

    public LinkedList<Instruction> getInstrList() {
        return this.instrList;
    }

    public void addInstr(Instruction instruction) {
        this.instrList.add(instruction);
    }

    public void setPreBlock(ArrayList<BasicBlock> block) {
        //如果该块中有phi指令，那需要维持preBlock的顺序
        boolean havePhi = false;
        if (instrList.getFirst() instanceof PhiInstr) {
            havePhi = true;
        }
        boolean allEqual = true;
        if (havePhi) {
            if (preBlock.size() == block.size()) {
                for (int count = 0; count < preBlock.size(); count++) {
                    BasicBlock block1 = preBlock.get(count);
                    if (!block.contains(block1)) {
                        allEqual = false;
                        break;
                    }
                }
                if (!allEqual) {
                    this.preBlock = block;
                    for (Instruction instruction : instrList) {
                        if (instruction instanceof PhiInstr) {
                            ((PhiInstr) instruction).setPreBlockList(block);
                        }
                    }
                }
                return;
            }
        }
        for (Instruction instruction : instrList) {
            if (instruction instanceof PhiInstr) {
                ((PhiInstr) instruction).setPreBlockList(block);
            }
        }
        this.preBlock = block;
    }

    public void setNextBlock(ArrayList<BasicBlock> block) {
        this.nextBlock = block;
    }

    public ArrayList<BasicBlock> getPreBlock() {
        return this.preBlock;
    }

    public ArrayList<BasicBlock> getNextBlock() {
        return this.nextBlock;
    }

    public void setDomBlock(ArrayList<BasicBlock> block) {
        this.domBlock = block;
    }

    public ArrayList<BasicBlock> getDomBlock() {
        return this.domBlock;
    }

    public void setParentBlock(BasicBlock block) {
        this.parentBlock = block;
    }

    public void setChildBlock(ArrayList<BasicBlock> block) {
        this.childBlock = block;
    }

    public void setDF(ArrayList<BasicBlock> DF) {
        this.DF = DF;
    }

    public BasicBlock getParentBlock() {
        return this.parentBlock;
    }

    public ArrayList<BasicBlock> getChildBlock() {
        return this.childBlock;
    }

    public ArrayList<BasicBlock> getDF() {
        return DF;
    }

    public Function getParentFunc() {
        return this.parentFunc;
    }

    public void setNextBlock(BasicBlock block) {
        if (!nextBlock.contains(block)) {
            this.nextBlock.add(block);
        }
    }

    public void setPreBlock(BasicBlock block) {
        if (!preBlock.contains(block)) {
            this.preBlock.add(block);
        }
    }

    public BasicBlock clone(Function function) {
        BasicBlock block = new BasicBlock(UndefinedType.undefined, function);
        for (Instruction instruction : instrList) {
            Instruction instruction1 = instruction.clone(block);
            instruction1.setBasicBlock(block);
            block.addInstr(instruction1);
        }
        return block;
    }

    public boolean isLegalToHoistInto() {
        return false;
    }
}
