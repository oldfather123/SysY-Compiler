package Backend.component;

import midend.BasicBlock;
import midend.Value;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;

public class AsmBlock extends AsmComponent {
    private final String name;
    private final LinkedList<AsmInstr> instrList;
    private final ArrayList<AsmBlock> preBlock, nextBlock;
    private int depth;

    public AsmBlock(String name, BasicBlock block) {
        this.name = name;
        this.instrList = new LinkedList<>();
        this.preBlock = new ArrayList<>();
        this.nextBlock = new ArrayList<>();
        setDepth(block);
    }

    private void setDepth(BasicBlock block) {
        this.depth = block.getLoopDepth();
    }

    public int getDepth() {
        return depth;
    }

    public String getName() {
        return name;
    }

    public LinkedList<AsmInstr> getInstrList() {
        return instrList;
    }

    public void addInstr(AsmInstr inst) {
        instrList.add(inst);
    }

    public void insertHead(AsmInstr inst) {
        instrList.addFirst(inst);
    }

    public void setPreNxtBlocks(HashMap<Value, AsmComponent> map, ArrayList<BasicBlock> preBlock, ArrayList<BasicBlock> nxtBlock) {
        preBlock.forEach(block -> this.preBlock.add((AsmBlock) map.get(block)));
        nxtBlock.forEach(block -> this.nextBlock.add((AsmBlock) map.get(block)));
    }

    public ArrayList<AsmBlock> getPreBlocks() {
        return preBlock;
    }

    public ArrayList<AsmBlock> getNextBlocks() {
        return nextBlock;
    }

    public void insertAfter(AsmInstr pos_instr, AsmInstr instr) {
        int index = instrList.indexOf(pos_instr);
        instrList.add(index + 1, instr);
    }

    public void insertBefore(AsmInstr pos_instr, AsmInstr instr) {
        int index = instrList.indexOf(pos_instr);
        instrList.add(index, instr);
    }

    public void deleteInstr(AsmInstr instr) {
        instrList.remove(instr);
    }
}