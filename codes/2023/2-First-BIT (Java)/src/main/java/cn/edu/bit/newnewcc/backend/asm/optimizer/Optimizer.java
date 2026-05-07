package cn.edu.bit.newnewcc.backend.asm.optimizer;

import cn.edu.bit.newnewcc.backend.asm.AsmFunction;
import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;

import java.util.List;

public interface Optimizer {
    boolean runOn(AsmFunction function);
}
