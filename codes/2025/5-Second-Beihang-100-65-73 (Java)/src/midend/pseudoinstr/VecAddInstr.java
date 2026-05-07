package midend.pseudoinstr;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.structure.BasicBlock;

import java.util.HashSet;

public class VecAddInstr extends Instruction {
    private Value op1;
    private Value op2;

    public VecAddInstr(Value op1, Value op2, Integer regIndex, BasicBlock parentBB) {
        super(op1.getType(), regIndex, parentBB);

        this.op1 = op1;
        this.op2 = op2;

        setUse(op1);
        setUse(op2);
    }

    public Value getOp1() {
        return op1;
    }

    public Value getOp2() {
        return op2;
    }

    @Override
    public HashSet<Value> getOperands() {
        HashSet<Value> operands = new HashSet<>();
        operands.add(op1);
        operands.add(op2);
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
        if (this.op1 == from) {
            this.op1 = to;
        }
        if (this.op2 == from) {
            this.op2 = to;
        }
    }

    @Override
    public String toString() {
        return "VEC_ADD " + this.getType().printIRType() + " " + this.value2string() + ", "
                + this.op1.getType().printIRType() + " "
                + this.op1.value2string() + " + " + this.op2.value2string();
    }
}
