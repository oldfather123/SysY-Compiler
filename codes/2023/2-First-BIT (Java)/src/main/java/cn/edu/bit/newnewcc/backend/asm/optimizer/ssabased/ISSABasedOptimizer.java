package cn.edu.bit.newnewcc.backend.asm.optimizer.ssabased;

import cn.edu.bit.newnewcc.backend.asm.instruction.AsmInstruction;
import cn.edu.bit.newnewcc.backend.asm.optimizer.SSABasedOptimizer;

public interface ISSABasedOptimizer {

    /**
     * 获取一条指令优化后的结果(instList, finalRegisterMap) <br>
     * instList：计算该结果所需要的新指令 <br>
     * finalRegisterMap(regA, regB) 表示优化后，需要将regA的使用替换为regB，Map的两侧必须都是虚拟寄存器  <br>
     *
     * @param instruction 待优化的指令
     * @return 若成功优化，返回二元组(instList, finalRegisterMap)；若无法优化，返回null
     */
    OptimizeResult getReplacement(SSABasedOptimizer ssaBasedOptimizer, AsmInstruction instruction);

    /**
     * 告知优化器函数开始
     */
    default void setFunctionBegins() {
    }

    /**
     * 告知优化器函数结束
     */
    default void setFunctionEnds() {
    }

    /**
     * 告知优化器一个基本块开始
     */
    default void setBlockBegins() {
    }

    /**
     * 告知优化器上一个基本块结束
     */
    default void setBlockEnds() {
    }

}
