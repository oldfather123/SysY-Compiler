package frontend.ir.instr.otherop;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.objecttype.TVoid;
import frontend.ir.structure.BasicBlock;

import java.util.HashSet;

public class MoveInstr extends Instruction {
    private final Value src;
    private final Value dst;

    public MoveInstr(Value src, Value dst, BasicBlock parentBB) {
        super(new TVoid(), -1, parentBB);
        this.src = src;
        this.dst = dst;
    }

    public Value getSrc() {
        return src;
    }

    public Value getDst() {
        return dst;
    }

    @Override
    protected void modifyValue(Value from, Value to) {
        throw new RuntimeException("不应该再改了");
    }

    @Override
    public String toString() {
        return "move " + src.value2string() + " --> " + dst.value2string();
    }

    @Override
    public String myHash() {
        return Integer.toString(this.hashCode());
    }

    @Override
    public Value simplify() {
        return null;
    }

    public HashSet<Value> getOperands() {
        HashSet<Value> operands = new HashSet<>();
        operands.add(src);
        return operands;
    }
}
