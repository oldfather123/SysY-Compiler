package backend.asm.instr.rv;

import backend.asm.instr.ASMInstruction;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

public abstract class RVIns extends ASMInstruction {
    protected RVIns(ASMBasicBlock parentBlock, Reg register) {
        super(parentBlock, register);
    }
}
