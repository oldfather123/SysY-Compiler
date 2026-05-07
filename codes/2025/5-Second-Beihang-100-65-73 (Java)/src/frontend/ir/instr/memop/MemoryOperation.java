package frontend.ir.instr.memop;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.objecttype.Type;
import frontend.ir.structure.BasicBlock;

import java.util.HashSet;

public abstract class MemoryOperation extends Instruction {

    protected MemoryOperation(Type type, Integer regIdx, BasicBlock parentBB) {
        super(type, regIdx, parentBB);
    }

    protected MemoryOperation(Type type, BasicBlock parentBB) {
        super(type, null, parentBB);
    }

    public abstract HashSet<Value> getOperands();
}
