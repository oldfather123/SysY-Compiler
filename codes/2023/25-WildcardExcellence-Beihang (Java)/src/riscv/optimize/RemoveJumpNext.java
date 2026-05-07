package riscv.optimize;

import riscv.value.AsmBasicBlock;
import riscv.value.AsmInstr;
import riscv.value.AsmModule;

import java.util.ArrayList;

// 移除不必要的 j 指令，即跳转到下一条语句的 j 指令
// 进行此优化之前，每个基本块都以 j 或 ret 指令结尾
public class RemoveJumpNext {
    public static void optimize(AsmModule module) {
        for (var func : module.funcMap.values()) {
            ArrayList<AsmBasicBlock> bbList = func.bbList;
            for (int i = 0; i < bbList.size() - 1; i++) {
                AsmBasicBlock bb = bbList.get(i);
                AsmBasicBlock nextBB = bbList.get(i + 1);
                if (bb.instrs.lastElement() instanceof AsmInstr.J jump) {
                    if (nextBB == jump.bb) {
                        bb.instrs.pollLast();
                    }
                }
            }
        }
    }
}
