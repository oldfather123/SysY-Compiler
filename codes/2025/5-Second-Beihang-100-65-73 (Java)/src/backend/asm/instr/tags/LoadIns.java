package backend.asm.instr.tags;

import backend.asm.ASMValue;
import backend.asm.immediate.ASMImmediate;

public interface LoadIns {
    ASMImmediate getOffset();

    ASMValue getBase();
}
