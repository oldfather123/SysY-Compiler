package ir.instr;

import ir.IrInstr;
import ir.type.IrType;
import ir.type.*;
import ir.value.Value;

import java.util.ArrayList;

import static utils.Panic.panicIfFalse;

// 截断指令
public class TruncInstr extends IrInstr {
    private Value result;
    private Value value;
    private final IrType fromType;
    public final IrType toType;

    public TruncInstr(Value result, IrType fromType, Value value, IrType toType) {
        panicIfFalse(checkBitWidth(fromType) > checkBitWidth(toType));
        this.result = result;
        this.fromType = fromType;
        this.value = value;
        this.toType = toType;
    }

    public void resetIntermediate(Value check, Value value) {
        if (this.value.getName().equals(check.getName())) {
            this.value = value;
        }
    }

    @Override
    public ArrayList<Value> getDependentValues() {
        return new ArrayList<Value>() {{
            add(value);
        }};
    }

    @Override
    public Value getInitialValue() {
        return result;
    }

    @Override
    public OpCode getOpCode() {
        return OpCode.TRUNC;
    }

    @Override
    public String toString() {
        return String.format("%s = trunc %s %s to %s", result.getName(), fromType, value.getName(), toType);
    }

    private int checkBitWidth(IrType type) {
        if (type instanceof IntegerType) {
            return ((IntegerType) type).getBits();
        } else if (type instanceof FloatType) {
            return ((FloatType) type).getBits();
        } else {
            panicIfFalse(false, "TruncInstr: checkBitWidth");
            return 0;
        }
    }

    public Value[] getOperands() {
        return new Value[]{result, value};
    }

    public void replaceOperand(Value old, Value newv) {
        if (IrInstr.checkReplace(old, value)) {
            value = newv;
        }
        if (IrInstr.checkReplace(old, result)) {
            result = newv;
        }
    }
}
