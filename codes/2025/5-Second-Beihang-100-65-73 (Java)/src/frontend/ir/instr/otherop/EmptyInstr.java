package frontend.ir.instr.otherop;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.objecttype.Type;
import frontend.ir.structure.BasicBlock;

import java.util.HashSet;

/**
 * 无实际意义，用来充当临时变量（占位）的指令
 */
public class EmptyInstr extends Instruction {
    public EmptyInstr(Type type, BasicBlock parentBB) {
        super(type, -1, parentBB);
    }

    @Override
    protected void modifyValue(Value from, Value to) {
        throw new RuntimeException("不应该再改了");
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
        return new HashSet<>();
    }
}
