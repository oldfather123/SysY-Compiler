package riscv.regalloc;

import midend.value.Value;
import riscv.value.AsmReg;

class Interval {
    int start, end;
    final Value reg; // 只有 reg 或 gep 指令的目标 loc 需要分配
    AsmReg asmReg = null; // 为 null 表示 spill 到栈上

    Interval(Value reg, int start, int end) {
        assert reg != null;
        this.reg = reg;
        this.start = start;
        this.end = end;
    }

    // spill 权重；权重低的优先溢出到栈上
    float calcSpillWeight() {
        return 1.0f / (end - start + 1); // 加 1 避免除 0
    }

    @Override
    public String toString() {
        String allocInfo = asmReg == null ? "spill" : asmReg.name();
        return String.format("%s(%d, %d, %s)", reg.getName(), start, end, allocInfo);
    }
}
