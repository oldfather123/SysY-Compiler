package ir.value;

import ir.IrInstr;
import ir.instr.*;

import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;

public class BasicBlock extends Value{
    private LinkedList<IrInstr> instr;

    public BasicBlock(String name) {
        super(name, null);
        instr = new LinkedList<>();
    }

    public void insertHeadInstr(IrInstr instr){
        this.instr.addFirst(instr);
    }

    public void insertTailInstr(IrInstr instr){
        this.instr.add(instr);
    }

    public void insertInstrBeforeLast(IrInstr instr) {
        IrInstr last = this.instr.removeLast();
        this.instr.add(instr);
        this.instr.add(last);
    }

    public boolean hasTerminator(){
        if(instr.isEmpty()){
            return false;
        }
        IrInstr last = instr.getLast();
        return last instanceof TermInstr && ((TermInstr) last).op != IrInstr.OpCode.CALL;
    }

    public IrInstr getTerminator() {
        if (instr.isEmpty()) {
            return null;
        }
        IrInstr last = instr.getLast();
        if (last instanceof TermInstr) {
            return last;
        }
        return null;
    }

    public LinkedList<IrInstr> getInstrs() {
        return instr;
    }

    public void resetInstrs(ArrayList<IrInstr> instrs){
        instr.clear();
        instr.addAll(instrs);
    }

    public TermInstr getReturn(){
        if(instr.isEmpty()){
            return null;
        }
        IrInstr last=instr.getLast();
        if(last instanceof TermInstr){
            if (((TermInstr) last).getOpType().contains("ret"))
                return (TermInstr)last;
        }
        return null;
    }

    @Override
    public boolean equals(Object obj){
        if(obj instanceof BasicBlock){
            BasicBlock block = (BasicBlock) obj;
            if (!block.getName().equals(this.getName()) || block.instr.size() != instr.size()) {
                return false;
            } else {
//                boolean flag = true;
//                for (int i = 0; i < block.instr.size(); i++) {
//                    if (!instr.get(i).equals(block.instr.get(i))) {
//                        flag = false;
//                        break;
//                    }
//                }
//                return flag;
                return true;
            }
        }
        return false;
    }

    @Override
    public String toString(){
        return getName();
    }

    @Override
    public int hashCode() {
        return getName().hashCode();
    }

    public BasicBlock[] getSuccessors() {
        if (hasTerminator()) {
            TermInstr term = (TermInstr) getTerminator();
            return term.getJumpTargets();
        }
        return new BasicBlock[0];
    }
}
