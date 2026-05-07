package midend.pseudoinstr;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.objecttype.derived.TVector;
import frontend.ir.structure.BasicBlock;

import java.util.HashSet;

/**
 * 将一个标量值复制到向量寄存器的所有 lanes（元素）中
 */
public class VecDuplicateInstr extends Instruction {
    private Value scalarValue;

    public VecDuplicateInstr(Value scalarValue, Integer regIndex, BasicBlock parentBB) {
        super(new TVector(scalarValue.getType()), regIndex, parentBB);

        this.scalarValue = scalarValue;

        this.setUse(scalarValue);
    }

    public Value getScalarValue() {
        return scalarValue;
    }

    @Override
    public HashSet<Value> getOperands() {
        HashSet<Value> operands = new HashSet<>();
        operands.add(scalarValue);
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
        if (this.scalarValue == from) {
            this.scalarValue = to;
        }
    }

    @Override
    public String toString() {
        return "VEC_DUPLICATE " + this.getType().printIRType() + " " + this.value2string() + ", "
                + this.scalarValue.getType().printIRType() + " " + this.scalarValue.value2string();
    }
}
