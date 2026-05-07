package ir.instr;

import ir.IrInstr;
import ir.value.Value;

import java.util.ArrayList;

// 注意！Move 指令可以出现在 IR 中了，因为一些优化加上合并，比如 %2 = phi [%1, %b1]; 在合并块时，可能会有 %2 = %1
public class MoveInstr extends IrInstr {
    private Value src;
    private Value dest;
    private OpCode opCode = OpCode.MOVE;

    public MoveInstr(Value dest, Value src) {
        this.src = src;
        this.dest = dest;
    }

    @Override
    public String toString() {
        if (src.getType().is("i32")) {
            return String.format("%s = add i32 0, %s\t\t; move actually", dest.getName(), src.getName());
        } else if (src.getType().is("i1")) {
            return String.format("%s = add i1 0, %s\t\t; move actually", dest.getName(), src.getName());
        } else if (src.getType().is("float")) {
            return String.format("%s = fadd float 0.0, %s\t\t; move actually", dest.getName(), src.getName());
        } else if (src.getType().is("POINTER")) {
            return String.format("%s = getelementptr inbounds i32, ptr %s, i32 0; move actually", dest.getName(), src.getName());
        } else {
            return String.format("%s = add %s 0, %s\t\t; move actually", dest.getName(), src.getType().toString(), src.getName());
        }
    }

    @Override
    public ArrayList<Value> getDependentValues() {
        return new ArrayList<Value>() {{
            add(src);
        }};
    }

    @Override
    public OpCode getOpCode() {
        return opCode;
    }

    @Override
    public Value getInitialValue() {
        return dest;
    }

    public Value[] getOperands() {
        return new Value[]{dest, src};
    }

    public void replaceOperand(Value old, Value newv) {
        if (src.getName().equals(old.getName())) {
            src = newv;
        }
        if (dest.getName().equals(old.getName())) {
            dest = newv;
        }
    }
}
