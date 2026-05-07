package backend.asm.instr.tags;

import backend.asm.structure.ASMBasicBlock;

/**
 * 无条件跳转
 */
public interface JumpIns {
    ASMBasicBlock getTarget();
}
