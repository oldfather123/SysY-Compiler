package ir.instr;

import ir.IrInstr;
import ir.type.IrType;
import ir.type.PointerType;
import ir.value.Value;

import java.util.ArrayList;
import java.util.Arrays;

import static utils.Panic.panicIfFalse;
import static utils.Panic.panicIfTrue;

// 二元指令，支持整数和浮点数的加减乘除、位运算等
public class BinaInstr extends IrInstr {
    public Value val1;
    public Value val2;
    public Value result;
    public OpCode op;
    public final IrType type;
    // 表示 指针 + 整数 类型的计算
    public final boolean pointerOffset;

    public BinaInstr(Value val1, Value val2, OpCode op, Value result) {
        panicIfFalse(op == OpCode.ADD || op == OpCode.FADD || op == OpCode.SUB || op == OpCode.FSUB ||
                op == OpCode.MUL || op == OpCode.FMUL || op == OpCode.SDIV || op == OpCode.FDIV ||
                op == OpCode.SREM || op == OpCode.FREM || op == OpCode.SHL || op == OpCode.LSHR ||
                op == OpCode.ASHR || op == OpCode.AND || op == OpCode.OR,
                "err in BinaInstr, op should be ADD, FADD, SUB, FSUB, MUL, FMUL, SDIV, FDIV, SREM, FREM, SHL, LSHR, ASHR, AND, OR");
        if (val1.getType().is("POINTER") && val2.getType().is("i32")) {
            pointerOffset = true;
        } else if (val1.getType().is("i32") && val2.getType().is("POINTER")) {
            pointerOffset = true;
            Value tmp = val1;
            val1 = val2;
            val2 = tmp;
        } else {
            panicIfFalse(val1.getType().equals(val2.getType()), "type conflict: " + val1.getType() + " <-> " + val2.getType());
            pointerOffset = false;
        }
        type = val1.getType();
        this.val1 = val1;
        this.val2 = val2;
        this.op = op;
        this.result = result;
    }

    public void resetIntermediate(Value check, Value value) {
        if (this.val1.getName().equals(check.getName())) {
            this.val1 = value;
        }
        if (this.val2.getName().equals(check.getName())) {
            this.val2 = value;
        }
    }

    @Override
    public ArrayList<Value> getDependentValues() {
        return new ArrayList<>(Arrays.asList(val1, val2));
    }

    @Override
    public OpCode getOpCode() {
        return op;
    }

    @Override
    public Value getInitialValue() {
        return result;
    }

    @Override
    public String toString() {
        if (pointerOffset) {
            return String.format("%s = getelementptr inbounds %s, %s %s, %s %s",
                    result.getName(), ((PointerType) val1.getType()).getFinalBase(), val1.getType(), val1.getName(), val2.getType(), val2.getName());
        }
        return String.format("%s = %s %s %s, %s",
                result.getName(), op.toString().toLowerCase(), type, val1.getName(), val2.getName());
    }

    public Value[] getOperands() {
        return new Value[]{result, val1, val2};
    }

    public void replaceOperand(Value old, Value newv) {
        if (IrInstr.checkReplace(old, this.val1)) {
            this.val1 = newv;
        }
        if (IrInstr.checkReplace(old, this.val2)) {
            this.val2 = newv;
        }
        if (IrInstr.checkReplace(old, this.result)) {
            this.result = newv;
        }
    }
}
