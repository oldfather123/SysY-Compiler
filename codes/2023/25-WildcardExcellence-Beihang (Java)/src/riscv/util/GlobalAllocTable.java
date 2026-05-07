package riscv.util;

import midend.value.global.GlobalVariable;

import java.util.HashMap;

public class GlobalAllocTable {
    private final HashMap<GlobalVariable, Integer> varOffsetMap = new HashMap<>();
    private int currentTop = 8;  // gp[0] 被全局 ra 占用

    // 全局区不会有指针，因此不需要考虑 size 为 8 的指针的情况

    public void addGlobalVar(GlobalVariable globalVar) {
        if (varOffsetMap.containsKey(globalVar)) return;
        varOffsetMap.put(globalVar, currentTop);
        currentTop += 4;
    }

    public void addGlobalVarWithSize(GlobalVariable globalVar, int size) {
        assert size % 4 == 0;
        if (varOffsetMap.containsKey(globalVar)) return;
        varOffsetMap.put(globalVar, currentTop);
        currentTop += size;
    }

    public int getGlobalVarOffset(GlobalVariable globalVar) {
        return varOffsetMap.get(globalVar);
    }

    public int globalSize() {
        return currentTop;
    }
}
