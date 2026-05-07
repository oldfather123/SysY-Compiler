package frontend.ir.instr.memop;

import frontend.ir.Value;
import frontend.ir.objecttype.Type;
import frontend.ir.structure.BasicBlock;

import java.util.HashSet;

public class LoadInstr extends MemoryOperation {
    private Value pointer;

    public LoadInstr(int regIdx, Type type, Value pointer, BasicBlock parentBB) {
        super(type, regIdx, parentBB);
        this.pointer = pointer;

        if (parentBB.isNotClosed()) {
            this.setUse(pointer);
        }
    }

    public Value getPointer() {
        return pointer;
    }

    @Override
    protected void modifyValue(Value from, Value to) {
        if (this.pointer == from) {
            this.pointer = to;
        }
    }

    @Override
    public String toString() {
        return this.value2string() + " = load " + this.getType().printIRType() + ", "
                + this.pointer.getType().printIRType() + " " + this.pointer.value2string();
    }

    @Override
    public String myHash() {
        return Integer.toString(this.hashCode());
    }

    @Override
    public Value simplify() {
        return this;
    }

    public HashSet<Value> getOperands() {
        HashSet<Value> operands = new HashSet<>();
        operands.add(pointer);
        return operands;
    }
}
