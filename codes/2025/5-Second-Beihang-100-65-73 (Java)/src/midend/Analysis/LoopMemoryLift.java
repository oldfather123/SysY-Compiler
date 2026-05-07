package midend.Analysis;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.AtomaticAddInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.AliasAnalysis;
import midend.DCE.MemoryPass;
import midend.FunctionInfoCollector;

import java.util.*;

/**
 * LoopMemoryLift - 环路内内存操作提升优化
 * 将特定的 Load/Store 指令从循环体中移动到循环外部（前置或后置位置），以减少循环内部的内存访问次数。
 * 【依赖分析 Pass】：LCSSA, PureFuncAnalysis, AliasAnalysis（确保无别名冲突）
 */

public class LoopMemoryLift extends MemoryPass {
    private static int LIFTNUM = 2;
    private static final Map<Pair<Value, Value>, Boolean> aliasCache = new HashMap<>();

    public static void execute(List<Function> functions) {
        aliasCache.clear();
        for (Function function : functions) {
            if (function instanceof Function.LibFunc) continue;
            AliasAnalysis.curFunction = function;
            for (LoopInfo top : function.getAncientLoopInfo()) {
                int cnt = 0;
                while (cnt < LIFTNUM && canLift(top)) {
                    cnt++;
                }
            }
        }
    }

    private static boolean canLift(LoopInfo loop) {
        boolean changed = false;
        for (LoopInfo sub : loop.getChildLoop()) {
            changed |= canLift(sub);
        }

        HashSet<BasicBlock> loopBlocks = new HashSet<>();
        loop.getAllBlocks(loopBlocks);

        Set<LoadInstr> loads = new HashSet<>();
        Set<StoreInstr> stores = new HashSet<>();
        Set<AtomaticAddInstr> atomicAdds = new HashSet<>();

        for (BasicBlock bb : loopBlocks) {
            for (Instruction inst : bb.getInstrList()) {
                if (inst instanceof CallInstr call &&
                        !FunctionInfoCollector.getFuncInfo(call.getCallee()).canLiftInLICM()) return false;
                if (inst instanceof LoadInstr load) loads.add(load);
                else if (inst instanceof StoreInstr store) stores.add(store);
                else if (inst instanceof AtomaticAddInstr add) atomicAdds.add(add);
            }
        }

        Set<Instruction> preLift = new HashSet<>();
        Set<Instruction> postLift = new HashSet<>();

        for (StoreInstr store : stores) {
            if (isDefinedInsideLoop(store.getPointer(), loopBlocks)) continue;
            if (store.getValueToStore() instanceof Instruction val && loopBlocks.contains(val.getParentBB())) continue;
            if (mayAliasWithAny(store, stores) || mayAliasWithAny(store, loads) || mayAliasWithAny(store, atomicAdds))
                continue;
            preLift.add(store);
        }

        Set<StoreInstr> storePointerSet = new HashSet<>(stores);
        for (LoadInstr load : loads) {
            if (isDefinedInsideLoop(load.getPointer(), loopBlocks)) continue;
            if (mayAliasWithAny(load, storePointerSet) || mayAliasWithAny(load, atomicAdds)) continue;

            boolean usedInLoop = false;
            for (Value user : load.getUserList()) {
                if (user instanceof Instruction userInst && loopBlocks.contains(userInst.getParentBB())) {
                    usedInLoop = true;
                    break;
                }
            }

            if (usedInLoop) preLift.add(load);
            else postLift.add(load);
        }

        if (!preLift.isEmpty()) liftPre(loop, preLift);
        if (!postLift.isEmpty()) liftPost(loop, postLift);

        return !preLift.isEmpty() || !postLift.isEmpty() || changed;
    }

    private static boolean isDefinedInsideLoop(Value v, Set<BasicBlock> blocks) {
        return v instanceof Instruction inst && blocks.contains(inst.getParentBB());
    }

    private static boolean mayAliasWithAny(Value instr, Set<? extends Value> others) {
        Value addr1 = getMemPointer(instr);
        if (addr1 == null) return true;

        for (Value other : others) {
            if (instr == other) continue;
            Value addr2 = getMemPointer(other);
            if (addr2 == null) return true;

            if (mayAlias(addr1, addr2)) return true;
        }
        return false;
    }

    private static Value getMemPointer(Value v) {
        if (v instanceof LoadInstr l) return l.getPointer();
        if (v instanceof StoreInstr s) return s.getPointer();
        if (v instanceof AtomaticAddInstr a) return a.getPtr();
        return null;
    }

    private static boolean mayAlias(Value a, Value b) {
        var key = Pair.of(a, b);
        return aliasCache.computeIfAbsent(key, k -> !noAliasSensitiveWithOffset(k.first, k.second));
    }

    private static void liftPre(LoopInfo loop, Set<Instruction> insts) {
        BasicBlock preHeader = loop.getPreHeader();
        for (Instruction inst : insts) {
            inst.liteRemoveFromList();
            inst.setParentBB(preHeader);
            preHeader.insertInsBefore(inst, preHeader.getLastInstr());
        }
    }

    private static void liftPost(LoopInfo loop, Set<Instruction> insts) {
        if (loop.getExitTargets().size() != 1) return;
        BasicBlock exit = loop.getExitTarget();
        Instruction insertBefore = exit.getFirstInstr();
        while (insertBefore instanceof PhiInstr) {
            insertBefore = (Instruction) insertBefore.getNext();
        }
        for (Instruction inst : insts) {
            inst.liteRemoveFromList();
            inst.setParentBB(exit);
            inst.insertBefore(insertBefore);
        }
    }

    public static class Pair<A, B> {
        public final A first;
        public final B second;

        private Pair(A first, B second) {
            this.first = first;
            this.second = second;
        }

        public static <A, B> Pair<A, B> of(A a, B b) {
            return new Pair<>(a, b);
        }

        @Override
        public boolean equals(Object o) {
            if (this == o) return true;
            if (!(o instanceof Pair<?, ?> pair)) return false;
            return Objects.equals(first, pair.first) && Objects.equals(second, pair.second);
        }

        @Override
        public int hashCode() {
            return Objects.hash(first, second);
        }
    }
}
