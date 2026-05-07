package backend.asm.instr.tags;

import backend.asm.register.Reg;

public interface MoveIns {
    Reg getSrc();

    boolean isFrozen(); // 是否冻结，冻结的 MOVE 指令不允许被优化掉
}
