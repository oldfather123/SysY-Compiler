package frontend.ir.instr.otherop;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.objecttype.arithmetic.TInt;
import frontend.ir.structure.BasicBlock;

import java.util.HashSet;

public class AtomaticAddInstr extends Instruction {
    private Value ptr; // 指针
    private Value inc; // 增量

    public AtomaticAddInstr(Value ptr, Value inc, BasicBlock parentBB) {
        super(new TInt(), -1, parentBB);
        this.ptr = ptr;
        this.inc = inc;
        this.setUse(ptr);
        this.setUse(inc);
    }

    public String myHash() {
        return Integer.toString(this.hashCode());
    }

    public Value simplify() {
        return this;
    }

    protected void modifyValue(Value from, Value to) {
        if (this.ptr == from) {
            this.ptr = to;
        } else if (this.inc == from) {
            this.inc = to;
        } else {
            throw new RuntimeException("没有符合的被改值");
        }
    }

    public HashSet<Value> getOperands() {
        HashSet<Value> operands = new HashSet<>();
        operands.add(ptr);
        operands.add(inc);
        return operands;
    }

    public Value getPtr() {
        return ptr;
    }

    public Value getInc() {
        return inc;
    }

//    public String toString() {
//        return String.format("atomicrmw add %s %s, %s %s monotonic",
//                ptr.getType(), ptr.getDescriptor(), inc.getType(), inc.getDescriptor());
//    }

    public String toString() {
        return "atomicrmw add " +
                ptr.getType().printIRType() + " " + ptr.value2string() + ", " +
                inc.getType().printIRType() + " " + inc.value2string() + " monotonic";
    }
}
