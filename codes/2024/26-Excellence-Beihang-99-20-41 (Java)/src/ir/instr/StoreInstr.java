package ir.instr;

import ir.IrInstr;
import ir.type.IrType;
import ir.value.Value;

import java.util.ArrayList;

// 存储指令
public class StoreInstr extends IrInstr {
    public Value pointer;
    private Value value;
    private final IrType valueType;
    private final IrType pointerType;

    public StoreInstr(IrType valueType, Value value, IrType pointerType, Value pointer) {
        this.valueType = valueType;
        this.value = value;
        this.pointerType = pointerType;
        this.pointer = pointer;
    }

    public Value getPointer() {
        return pointer;
    }

    public Value getValue() {
        return value;
    }

    @Override
    public String toString() {
        return String.format("store %s %s, %s %s", valueType, value.getName(), pointerType, pointer.getName());
    }

    @Override
    public ArrayList<Value> getDependentValues() {
        return new ArrayList<Value>() {{
            add(pointer);
            add(value);
        }};
    }

    @Override
    public OpCode getOpCode() {
        return OpCode.STORE;
    }

    public void resetIntermediate(Value check, Value value) {
        if (this.value.getName().equals(check.getName())) {
            this.value = value;
        }
    }

    public Value[] getOperands() {
        return new Value[]{pointer, value};
    }

    public void replaceOperand(Value old, Value newv) {
        if (IrInstr.checkReplace(old, pointer)) {
            pointer = newv;
        }
        if (IrInstr.checkReplace(old, value)) {
            value = newv;
        }
    }

    public IrType getPointerType() {
        return pointerType;
    }

    public IrType getValueType() {
        return valueType;
    }
}
