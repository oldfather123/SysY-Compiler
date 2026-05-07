package midend.instr;

import midend.LLVMType.Type;
import midend.LLVMType.VoidType;
import midend.Value;

import java.util.ArrayList;
import java.util.List;

public class PCopyInstr extends Instruction {
    private ArrayList<Value> before = new ArrayList<>();
    private ArrayList<Value> now = new ArrayList<>();
    public PCopyInstr() {
        super(VoidType.voidType, "pcopy", InstrType.PCOPY);
    }

    public void addCopy(List<Value> values) {
        this.before.add(values.get(0));
        this.now.add(values.get(1));
    }

    public ArrayList<Value> getBefore() {
        return this.before;
    }

    public ArrayList<Value> getNow() {
        return this.now;
    }
}
