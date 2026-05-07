package midend.pseudoinstr;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.objecttype.derived.TVector;
import frontend.ir.structure.BasicBlock;

import java.util.Arrays;
import java.util.HashSet;
import java.util.List;

public class VecMoveFromScaInstr extends Instruction {
    private Value value0;
    private Value value1;
    private Value value2;
    private Value value3;

    public VecMoveFromScaInstr(Value value0, Value value1, Value value2, Value value3,
                               int regIdx, BasicBlock parentBB) {
        super(new TVector(value0.getType()), regIdx, parentBB);

        this.value0 = value0;
        this.value1 = value1;
        this.value2 = value2;
        this.value3 = value3;

        this.setUse(value0);
        this.setUse(value1);
        this.setUse(value2);
        this.setUse(value3);
    }

    public List<Value> getSrcValueList() {
        return Arrays.asList(value0, value1, value2, value3);
    }

    @Override
    public HashSet<Value> getOperands() {
        HashSet<Value> operands = new HashSet<>();
        operands.add(value0);
        operands.add(value1);
        operands.add(value2);
        operands.add(value3);
        return operands;
    }

    @Override
    public String myHash() {
        return String.valueOf(this.hashCode());
    }

    @Override
    public Value simplify() {
        return null;
    }

    @Override
    protected void modifyValue(Value from, Value to) {
        if (this.value0 == from) {
            this.value0 = to;
        }
        if (this.value1 == from) {
            this.value1 = to;
        }
        if (this.value2 == from) {
            this.value2 = to;
        }
        if (this.value3 == from) {
            this.value3 = to;
        }
    }

    @Override
    public String toString() {
        return "VEC_MOVE_FROM_SCA " + this.getType().printIRType() + " " + this.value2string()
                + ", " + this.value0.getType().printIRType() + " "
                + this.value0.value2string() + ", " + this.value1.value2string() + ", "
                + this.value2.value2string() + ", " + this.value3.value2string();
    }
}
