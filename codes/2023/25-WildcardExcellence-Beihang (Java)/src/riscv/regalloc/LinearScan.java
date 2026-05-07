package riscv.regalloc;

import midend.value.Function;
import midend.value.Value;
import midend.value.instrs.*;
import riscv.value.AsmFunction;
import riscv.value.AsmReg;
import util.Pair;

import java.util.*;

// 线性扫描法
// 不需要使用数据流分析，因为消 phi 后的 IR 已经有了精确的 def-use 信息
class LinearScan {
    static void runAllocForFunc(Function func, AsmFunction asmFunc, RegPool regPool, RegPool fRegPool) {
        // 需要记录活跃寄存器的行
        List<Pair<Integer, Instr>> rActiveInfoRequiringInstrs = new ArrayList<>();

        // 顺序标号，同时标记需要记录活跃寄存器的行
        int line = 1;
        for (var bbp : func.getBasicBlocks()) {
            for (var instrp : bbp.get().getInstrList()) {
                Instr instr = instrp.get();
                instr.number = line;
                if (instr instanceof Call || instr instanceof CallVoid) {
                    rActiveInfoRequiringInstrs.add(new Pair<>(line, instr));
                }
                line++;
            }
        }

        // 已分配寄存器且未结束的 interval
        Set<Interval> active = new HashSet<>();
        // 所有 interval，按开始时间升序排列
        List<Interval> intervalList = CalcIntervals.runCalc(func);

        for (var curInterval : intervalList) {
            // 寻找当前 interval 开始时已经结束（可以取等号）的 interval，将其从 active 中除去，并回收其寄存器
            active.removeIf(interval -> {
                // remove
                if (curInterval.start >= interval.end) {
                    AsmReg r = interval.asmReg;
                    if (AsmReg.isFloatReg(r)) fRegPool.free(r);
                    else regPool.free(r);
                    return true;
                }
                return false;
            });

            // 尝试分配一个寄存器
            AsmReg r;
            boolean acquireFloatReg = curInterval.reg.getType().isFloatType();
            if (acquireFloatReg)
                r = fRegPool.fetch();
            else
                r = regPool.fetch();

            // 分配成功
            if (r != null) {
                curInterval.asmReg = r;
                active.add(curInterval);
            }
            // 否则，做 spill
            else {
                // 对所有的 active，以及 curInterval，计算权重
                float minWeight = curInterval.calcSpillWeight();
                Interval minWeightInterval = curInterval;
                for (var interval : active) {
                    if (acquireFloatReg != AsmReg.isFloatReg(interval.asmReg)) continue;
                    float weight = interval.calcSpillWeight();
                    if (weight < minWeight) {
                        minWeight = weight;
                        minWeightInterval = interval;
                    }
                }
                // 溢出权重最小的 interval，curInterval 获得它的寄存器。如果 curInterval 权重最小，什么都不做
                if (minWeightInterval != curInterval) {
                    curInterval.asmReg = minWeightInterval.asmReg;
                    minWeightInterval.asmReg = null;
                    active.add(curInterval);
                    active.remove(minWeightInterval);
                }
            }
        }

        System.out.println(intervalList);

        // 填写 allocTable
        for (var interval : intervalList) {
            AsmReg r = interval.asmReg;
            if (r != null) {
                asmFunc.allocTable.addReg(interval.reg, interval.asmReg);
            }
            else {
                asmFunc.allocTable.addSpill(interval.reg);
            }
        }

        // 记录活跃寄存器
        // 开始时间 < 当前行 && 结束时间 > 当前行 的寄存器才需要保留
        for (var pair : rActiveInfoRequiringInstrs) {
            int callLine = pair.first;
            Instr callInstr = pair.second;
            List<AsmReg> rActiveList = new ArrayList<>();
            for (var interval : intervalList) {
                if (interval.start >= callLine) break;
                if (interval.end > callLine) {
                    if (interval.asmReg != null) {
                        rActiveList.add(interval.asmReg);
                    }
                }
            }
            asmFunc.allocTable.rActiveListMap.put(callInstr, rActiveList);
        }
    }

    static boolean isAllocable(Value value) {
        return value instanceof Reg || value instanceof GetElementPtr;
    }
}
