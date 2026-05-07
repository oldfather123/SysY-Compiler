package ir.instr;

import ir.IrInstr;
import ir.type.IrType;
import ir.type.PointerType;
import ir.type.Ty;
import ir.value.Value;

import java.util.ArrayList;

import static utils.Panic.panicIfFalse;

// 分配指令，用于分配内存
public class AllocaInstr extends IrInstr {
    public Value pointer;
    private final IrType pointerType;
    public final int size;
    private final IrType sizeType;
    private final int align;

    public AllocaInstr(Value pointer, IrType pointerType, int size, IrType sizeType, int align) {
        this.pointer = pointer;
        this.pointerType = pointerType;
        this.size = size;
        this.sizeType = sizeType;
        this.align = align;
    }

    public AllocaInstr(Value pointer, IrType pointerType, int size, IrType sizeType) {
        this.pointer = pointer;
        this.pointerType = pointerType;
        this.size = size;
        this.sizeType = sizeType;
        this.align = 0;
    }

    public AllocaInstr(Value pointer, IrType pointerType) {
        this.pointer = pointer;
        this.pointerType = pointerType;
        this.size = 1;
        this.sizeType = Ty.I32;
        this.align = 0;
    }

    public IrType getBaseType() {
        return ((PointerType) pointerType).getBaseType();
    }

    @Override
    public ArrayList<Value> getDependentValues() {
        return new ArrayList<>(1) {{
            add(pointer);
        }};
    }

    @Override
    public Value getInitialValue() {
        return pointer;
    }

    @Override
    public OpCode getOpCode() {
        return OpCode.ALLOCA;
    }

    @Override
    public String toString() {
        if (align > 0) {
            return String.format("%s = alloca %s, %s %d, align %d", pointer.getName(), pointerType, sizeType, size, align);
        }
        else {
            return String.format("%s = alloca %s, %s %d", pointer.getName(), pointerType, sizeType, size);
        }
    }

    public Value[] getOperands() {
        return new Value[]{pointer};
    }

    public void replaceOperand(Value old, Value newv) {
        if (pointer.getName().equals(old.getName())) {
            panicIfFalse(pointer.getType().equals(newv.getType()));
            pointer = newv;
        }
    }
}
