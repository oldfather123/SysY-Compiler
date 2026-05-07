package backend.asm.instr.tags;

import backend.asm.ASMValue;
import backend.asm.immediate.ASMImmediate;

public interface StoreIns {
    ASMImmediate getOffset();

    ASMValue getBase();

    ASMValue getValue();
}
