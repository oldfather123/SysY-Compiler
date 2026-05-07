package frontend.ir.instr.terminator;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.structure.BasicBlock;

import java.util.HashSet;

public abstract class Terminator extends Instruction {
    protected Terminator(BasicBlock parentBB) {
        super(null, null, parentBB);
    }

    public abstract void replaceJumpTarget(BasicBlock original, BasicBlock newTarget);

    @Override
    public void forceRemoveFromList() {
        this.getParentBB().open();
        super.forceRemoveFromList();
    }

    @Override
    public String myHash() {
        return Integer.toString(this.hashCode());
    }

    @Override
    public Value simplify() {
        return this;
    }

    public abstract HashSet<Value> getOperands();
}
