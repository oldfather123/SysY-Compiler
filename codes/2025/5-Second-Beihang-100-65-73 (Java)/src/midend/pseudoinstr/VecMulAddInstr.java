package midend.pseudoinstr;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.objecttype.derived.TVector;
import frontend.ir.structure.BasicBlock;

import java.util.HashSet;

public class VecMulAddInstr extends Instruction {
    private Value op1;
    private Value op2;
    private Value valToAdd;

    public VecMulAddInstr(Value op1, Value op2, Value valToAdd, Integer regIndex, BasicBlock parentBB) {
        super(new TVector(valToAdd.getType()), regIndex, parentBB);

        this.op1 = op1;
        this.op2 = op2;
        this.valToAdd = valToAdd;

        setUse(op1);
        setUse(op2);
        setUse(valToAdd);
    }

    public Value getOp1() {
        return op1;
    }

    public Value getOp2() {
        return op2;
    }

    public Value getValToAdd() {
        return valToAdd;
    }

    @Override
    public HashSet<Value> getOperands() {
        HashSet<Value> operands = new HashSet<>();
        operands.add(op1);
        operands.add(op2);
        operands.add(valToAdd);
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
        if (this.valToAdd == from) {
            this.valToAdd = to;
        }
    }

    @Override
    public String toString() {
        return "VEC_MUL_ADD " + this.getType().printIRType() + " " + this.value2string() + ", "
                + this.op1.getType().printIRType() + " "
                + this.op1.value2string() + " * " + this.op2.value2string() + " + " + this.valToAdd.value2string();
    }
}
