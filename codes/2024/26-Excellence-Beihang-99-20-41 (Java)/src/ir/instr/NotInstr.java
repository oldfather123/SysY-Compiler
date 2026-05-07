package ir.instr;

import ir.IrInstr;
import ir.value.Value;

import java.util.ArrayList;

import static utils.Panic.panicIfFalse;

// 取反指令
public class NotInstr extends IrInstr {
    public Value val;
    public Value result;

    public NotInstr(Value val, Value result) {
        this.val = val;
        this.result = result;
        panicIfFalse(val.getType().is("i1"), "NotInstr: val is not i1");
        panicIfFalse(result.getType().is("i1"), "NotInstr: result is not i1");
    }

    public void resetIntermediate(Value check, Value value) {
        if (this.val.getName().equals(check.getName())) {
            this.val = value;
        }
    }

    @Override
    public ArrayList<Value> getDependentValues() {
        return new ArrayList<Value>() {{
            add(val);
        }};
    }

    @Override
    public Value getInitialValue() {
        return result;
    }

    @Override
    public OpCode getOpCode() {
        return OpCode.NOT;
    }

    @Override
    public String toString() {
        return String.format("%s = xor %s %s, true  ; not actually", result.getName(), val.getType(), val.getName());
    }

    public Value[] getOperands() {
        return new Value[]{result, val};
    }

    public void replaceOperand(Value old, Value newv) {
        if (IrInstr.checkReplace(old, val)) {
            val = newv;
        }
        if (IrInstr.checkReplace(old, result)) {
            result = newv;
        }
    }
}
