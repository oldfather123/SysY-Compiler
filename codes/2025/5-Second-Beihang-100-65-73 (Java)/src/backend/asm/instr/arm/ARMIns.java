package backend.asm.instr.arm;

import backend.asm.instr.ASMInstruction;
import backend.asm.register.Reg;
import backend.asm.structure.ASMBasicBlock;

public abstract class ARMIns extends ASMInstruction {
    protected ARMIns(ASMBasicBlock parentBlock, Reg register) {
        super(parentBlock, register);
    }
}
