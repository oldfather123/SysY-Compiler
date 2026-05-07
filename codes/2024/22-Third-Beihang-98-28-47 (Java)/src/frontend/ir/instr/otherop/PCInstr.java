package frontend.ir.instr.otherop;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.structure.Function;
import frontend.ir.structure.Procedure;

import java.util.ArrayList;

public class PCInstr extends Instruction {
    private ArrayList<Value> srcs;
    private ArrayList<Value> dsts;

    public PCInstr() {
        srcs = new ArrayList<>();
        dsts = new ArrayList<>();
    }

    public void addPC(Value src, Value dst) {
        srcs.add(src);
        dsts.add(dst);
    }

    public ArrayList<Value> getSrcs() {
        return srcs;
    }

    public ArrayList<Value> getDsts() {
        return dsts;
    }

    @Override
    public Number getNumber() {
        return null;
    }

    @Override
    public DataType getDataType() {
        return null;
    }

    @Override
    public String print() {
        return null;
    }

    @Override
    public void modifyValue(Value from, Value to) {
        throw new RuntimeException("?");
    }

    @Override
    public String myHash() {
        return null;
    }

    @Override
    public Value operationSimplify() {
        return null;
    }
    
    @Override
    public Instruction cloneShell(Procedure procedure) {
        return new PCInstr();
    }
}
