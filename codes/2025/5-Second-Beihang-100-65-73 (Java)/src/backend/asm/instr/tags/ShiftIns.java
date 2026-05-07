package backend.asm.instr.tags;

import backend.asm.immediate.ASMImmediate;

public interface ShiftIns {
    ASMImmediate getShiftImm();

    Type getType();

    enum Type {
        LOGICAL_LEFT,
        LOGICAL_RIGHT,
        ARITHMETIC_RIGHT,
    }
}
