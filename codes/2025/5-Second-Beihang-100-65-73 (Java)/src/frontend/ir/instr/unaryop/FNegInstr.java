package frontend.ir.instr.unaryop;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.objecttype.arithmetic.TFloat;
import frontend.ir.structure.BasicBlock;

import java.util.HashSet;

public class FNegInstr extends Instruction {
    private Value value;

    public FNegInstr(Integer regIdx, Value value, BasicBlock parentBB) {
        super(new TFloat(), regIdx, parentBB);
        if (value == null || !(value.getType() instanceof TFloat)) {
            throw new RuntimeException("Float only!");
        }
        this.value = value;
        this.setUse(value);
    }

    public Value getValue() {
        return value;
    }

    @Override
    public String toString() {
        return this.value2string() + " = fneg float " + value.value2string();
    }

    @Override
    public String myHash() {
        return "fneg " + value.value2string();
    }

    @Override
    public Value simplify() {
        return this;
    }

    @Override
    protected void modifyValue(Value from, Value to) {
        if (value == from) {
            value = to;
        } else {
            throw new RuntimeException("Only one value here");
        }
    }

    public HashSet<Value> getOperands() {
        HashSet<Value> operands = new HashSet<>();
        operands.add(value);
        return operands;
    }
}
