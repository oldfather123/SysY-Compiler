package frontend.ir.instr.terminator;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;

public abstract class Terminator extends Instruction {
    @Override
    public Value operationSimplify() {
        return null;
    }
    
    @Override
    public Number getNumber() {
        return -1;
    }
    
    @Override
    public String myHash() {
        return Integer.toString(this.hashCode());
    }
}
