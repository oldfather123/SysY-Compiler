package backend.asm.instr.tags;

import backend.asm.structure.ASMFunction;

/**
 * 用来调用函数（跳转到函数标签）的指令
 */
public interface CallIns {
    ASMFunction getTargetFunc();
}
