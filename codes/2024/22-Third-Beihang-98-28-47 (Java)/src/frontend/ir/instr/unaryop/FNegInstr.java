package frontend.ir.instr.unaryop;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstFloat;
import frontend.ir.constvalue.ConstValue;
import frontend.ir.instr.Instruction;
import frontend.ir.structure.Function;
import frontend.ir.structure.Procedure;

import java.util.Objects;

public class FNegInstr extends Instruction {
    private final int result;
    private Value value;
    
    public FNegInstr(int result, Value value) {
        this.result = result;
        this.value = value;
        setUse(value);
    }
    
    @Override
    public Number getNumber() {
        return result;
    }
    
    @Override
    public DataType getDataType() {
        return DataType.FLOAT;
    }
    
    @Override
    public String print() {
        return "%reg_" + result + " = fneg float " + value.value2string();
    }

    @Override
    public void modifyValue(Value from, Value to) {
        if (value == from) {
            value = to;
        } else {
            throw new RuntimeException();
        }
    }
    
    @Override
    public String myHash() {
        return "fneg" + value.value2string();
    }
    
    @Override
    public Value operationSimplify() {
        if (this.value instanceof ConstValue) {
            return new ConstFloat(-1 * value.getNumber().floatValue());
        }
        return null;
    }
    
    @Override
    public Instruction cloneShell(Procedure procedure) {
        return new FNegInstr(procedure.getAndAddRegIndex(), value);
    }
    
    public Value getValue() {
        return value;
    }
}
