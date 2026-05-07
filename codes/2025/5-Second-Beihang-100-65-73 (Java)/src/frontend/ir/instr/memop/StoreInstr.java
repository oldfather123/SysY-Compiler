package frontend.ir.instr.memop;

import frontend.ir.Value;
import frontend.ir.structure.BasicBlock;

import java.util.HashSet;

public class StoreInstr extends MemoryOperation {
    private Value valueToStore;
    private Value pointer;
    
    public StoreInstr(Value valueToStore, Value pointer, BasicBlock parentBB) {
        super(null, parentBB);
        this.valueToStore = valueToStore;
        this.pointer = pointer;
        
        if (parentBB.isNotClosed()) {
            this.setUse(valueToStore);
            this.setUse(pointer);
        }
    }
    
    public Value getValueToStore() {
        return valueToStore;
    }
    
    public Value getPointer() {
        return pointer;
    }
    
    @Override
    protected void modifyValue(Value from, Value to) {
        if (this.valueToStore == from) {
            this.valueToStore = to;
        }
        if (this.pointer == from) {
            this.pointer = to;
        }
    }
    
    @Override
    public String toString() {
        return "store " + this.valueToStore.getType().printIRType() + " " + this.valueToStore.value2string()
                + ", " + this.pointer.getType().printIRType() + " " + this.pointer.value2string();
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
        operands.add(valueToStore);
        operands.add(pointer);
        return operands;
    }
}
