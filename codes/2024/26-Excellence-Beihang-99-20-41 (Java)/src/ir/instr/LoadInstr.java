package ir.instr;

import ir.IrInstr;
import ir.type.IrType;
import ir.value.Value;
import utils.Panic;

import java.util.ArrayList;

// 加载指令
public class LoadInstr extends IrInstr {
    public Value pointer;
    public Value result;
    private final IrType pointerType;
    public final IrType resultType;

    public LoadInstr(Value result, IrType resultType, IrType pointerType, Value pointer) {
        Panic.panicIfFalse(result.getType().equals(resultType), "Unmatched result type");
        this.resultType = resultType;
        this.pointerType = pointerType;
        this.pointer = pointer;
        this.result = result;
    }

    @Override
    public ArrayList<Value> getDependentValues() {
        return new ArrayList<Value>() {{
            add(pointer);
        }};
    }

    @Override
    public OpCode getOpCode() {
        return OpCode.LOAD;
    }

    @Override
    public Value getInitialValue() {
        return result;
    }

    public Value getPointer() {
        return pointer;
    }

    @Override
    public String toString() {
        return String.format("%s = load %s, %s %s", result.getName(), resultType, pointerType, pointer.getName());
    }

    public Value[] getOperands() {
        return new Value[]{result, pointer};
    }

    public void replaceOperand(Value old, Value newv) {
        if (IrInstr.checkReplace(old, pointer)) {
            pointer = newv;
        }
        if (IrInstr.checkReplace(old, result)) {
            result = newv;
        }
    }
}
