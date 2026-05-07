package ir.instr;

import ir.IrInstr;
import ir.type.IrType;
import ir.value.Value;

import java.util.ArrayList;

import static utils.Panic.panicIfFalse;

// 整数转浮点数指令
public class I2fInstr extends IrInstr{
    // 整型转浮点
    Value from;
    Value to;
    IrType fromType;
    IrType toType;
    IrInstr.OpCode opCode = IrInstr.OpCode.SITOFP;

    public I2fInstr(Value from, Value to) {
        panicIfFalse(from.getType().is("INT"));
        panicIfFalse(to.getType().is("FLOAT"));
        this.from = from;
        this.fromType = from.getType();
        this.to = to;
        this.toType = to.getType();
    }


    public void resetIntermediate(Value check, Value value) {
        if (this.from.getName().equals(check.getName())) {
            this.from = value;
        }
    }

    @Override
    public ArrayList<Value> getDependentValues() {
        return new ArrayList<Value>() {{
            add(from);
        }};
    }

    @Override
    public OpCode getOpCode() {
        return opCode;
    }

    @Override
    public Value getInitialValue() {
        return to;
    }

    @Override
    public String toString() {
        return String.format("%s = sitofp %s %s to %s",
                to.getName(), fromType, from.getName(), toType);
    }

    public Value[] getOperands() {
        return new Value[]{to, from};
    }

    public void replaceOperand(Value old, Value newv) {
        if (IrInstr.checkReplace(old, from)) {
            from = newv;
        }
        if (IrInstr.checkReplace(old, to)) {
            to = newv;
        }
    }
}
