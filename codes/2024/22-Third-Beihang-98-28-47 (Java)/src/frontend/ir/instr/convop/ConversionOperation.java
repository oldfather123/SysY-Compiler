package frontend.ir.instr.convop;

import frontend.ir.DataType;
import frontend.ir.Value;
import frontend.ir.instr.Instruction;

public abstract class ConversionOperation extends Instruction {
    private final int result;
    private final DataType from;
    private final DataType to;
    protected Value value;
    private final String opName;
    
    public ConversionOperation(int result, DataType from, DataType to, Value value, String name) {
        this.result = result;
        this.from = from;
        this.to     = to;
        this.value  = value;
        this.opName = name;
        this.pointerLevel = value.getPointerLevel();
        setUse(value);
    }
    
    @Override
    public Number getNumber() {
        return result;
    }
    
    @Override
    public DataType getDataType() {
        return to;
    }
    
    @Override
    public String print() {
        String fromType;
        if (this.pointerLevel == 0) {
            fromType = from.toString();
        } else {
            fromType = value.type2string();
        }
        return "%reg_" + result + " = " +
                opName + " " + fromType + " " +
                value.value2string() + " to " + to;
    }

    @Override
    public void modifyValue(Value from, Value to) {
        if (value == from) {
            value = to;
        } else {
            throw new RuntimeException();
        }
    }

    public Value getValue() {
        return value;
    }

    public String getOpName() {
        return opName;
    }
    
    @Override
    public String myHash() {
        return value.value2string() + opName;
    }
}
