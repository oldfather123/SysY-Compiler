package backend.asm.instr.tags;

import backend.asm.structure.ASMBasicBlock;

/**
 * 有条件跳转
 */
public interface BranchIns {
    ASMBasicBlock getTarget();
}
