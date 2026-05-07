package riscv.regalloc;

import midend.value.BasicBlock;
import midend.value.Function;
import midend.value.Value;
import midend.value.instrs.Instr;

import java.util.*;

class CalcIntervals {
    // 不记录 reg 信息
    private static final class LiveRange {
        int start;
        int end;

        LiveRange(int start, int end) {
            this.start = start;
            this.end = end;
        }
    }

    private static Map<Value, List<LiveRange>> map;

    static List<Interval> runCalc(Function func) {
        map = new HashMap<>();

        LivenessAnalysis.runAnalysis(func);

        func.getBasicBlocks().forEachReverse(bbp -> {
            BasicBlock bb = bbp.get();
            int blockStart = bb.getInstrList().firstElement().number;
            int blockEnd = bb.getInstrList().lastElement().number;

            // 基本块的 outReg ，先假定在在整个块内都活跃
            bb.outRegs.forEach(out -> addRange(out, blockStart, blockEnd + 1));

            // 细化活跃区间
            bb.getInstrList().forEachReverse(instrp -> {
                Instr instr = instrp.get();
                // def
                if (LinearScan.isAllocable(instr)) {
                    if (map.containsKey(instr)) {
                        List<LiveRange> ranges = map.get(instr);
                        // def 值应当尽量靠后
                        if (ranges.get(0).start < instr.number) {
                            ranges.get(0).start = instr.number;
                        }
                    }
                    // 这是一个无用赋值；保留到这一阶段还存在说明这是一个有副作用的函数的返回值
                    else {
                        addRange(instr, instr.number, instr.number);
                    }
                }
                // use
                for (var opr : instr.getOperandList()) {
                    if (LinearScan.isAllocable(opr)) {
                        addRange(opr, blockStart, instr.number);
                    }
                }
            });
        });

        // range to interval
        List<Interval> result = new ArrayList<>();

        for (var e : map.entrySet()) {
            Value reg = e.getKey();
            List<LiveRange> rangeList = e.getValue();
            int start = rangeList.get(0).start;
            int end = rangeList.get(0).end;
            for (int i = 1; i < rangeList.size(); i++) {
                start = Math.min(start, rangeList.get(i).start);
                end = Math.max(end, rangeList.get(i).end);
            }
            result.add(new Interval(reg, start, end));
        }
        result.sort(Comparator.comparingInt(interval -> interval.start));

        return result;
    }


    private static void addRange(Value reg, int start, int end) {
        if (map.containsKey(reg)) {
            map.get(reg).add(new LiveRange(start, end));
        }
        else {
            map.put(reg, new ArrayList<>(List.of(new LiveRange(start, end))));
        }
    }
}
