package ir.instr;

import ir.IrInstr;
import ir.type.FloatType;
import ir.type.IrType;
import ir.type.IntegerType;
import ir.value.Value;

import java.util.ArrayList;

import static utils.Panic.panicIfFalse;

// 扩展指令，支持符号扩展和无符号扩展
public class ExtInstr extends IrInstr {
    private Value result;
    private Value value;
    public final IrType fromType;
    public final IrType toType;
    public final boolean isSigned;

    public ExtInstr(Value result, IrType fromType, Value value, IrType toType, boolean isSigned) {
        panicIfFalse(checkBitWidth(fromType) < checkBitWidth(toType));
        panicIfFalse(fromType instanceof IntegerType);
        panicIfFalse(toType instanceof IntegerType);
        this.result = result;
        this.fromType = fromType;
        this.value = value;
        this.toType = toType;
        this.isSigned = isSigned;
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
    public OpCode getOpCode() {
        return isSigned ? OpCode.SEXT : OpCode.ZEXT;
    }

    @Override
    public Value getInitialValue() {
        return result;
    }

    @Override
    public String toString() {
        if (isSigned)
            return String.format("%s = sext %s %s to %s", result.getName(), fromType, value.getName(), toType);
        else
            return String.format("%s = zext %s %s to %s", result.getName(), fromType, value.getName(), toType);
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
