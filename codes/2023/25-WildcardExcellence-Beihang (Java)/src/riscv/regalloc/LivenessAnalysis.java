package riscv.regalloc;

import midend.value.BasicBlock;
import midend.value.Function;
import midend.value.Value;
import midend.value.instrs.Instr;
import util.nodelist.Node;

import java.util.HashSet;
import java.util.Set;

class LivenessAnalysis {
    static void runAnalysis(Function func) {
        for (var bbp : func.getBasicBlocks()) {
            BasicBlock bb = bbp.get();
            bb.useRegs = new HashSet<>();
            bb.defRegs = new HashSet<>();
            bb.inRegs = new HashSet<>();
            bb.outRegs = new HashSet<>();

            for (var instrp : bb.getInstrList()) {
                Instr instr = instrp.get();
                // use
                for (var opr : instr.getOperandList()) {
                    if (LinearScan.isAllocable(opr)) {
                        if (!bb.defRegs.contains(opr)) {
                            bb.useRegs.add(opr);
                        }
                    }
                }
                // def
                if (LinearScan.isAllocable(instr)) {
                    if (!bb.useRegs.contains(instr)) {
                        bb.defRegs.add(instr);
                    }
                }
            }
        }

        // 迭代计算 in, out
        boolean changed = true;
        while (changed) {
            changed = false;
            // 反向遍历 bb
            for (Node<BasicBlock> bbp = func.getBasicBlocks().lastNode(); bbp != func.getBasicBlocks().headNode(); bbp = bbp.prev()) {
                BasicBlock bb = bbp.get();
                // out = ∪ (in of succ)
                if (bb.getSucBB() != null) {
                    bb.getSucBB().forEach(succ -> bb.outRegs.addAll(succ.inRegs));
                }
                // in = use ∪ (out - def)
                Set<Value> inAns = new HashSet<>(bb.outRegs);
                inAns.retainAll(bb.defRegs);
                inAns.addAll(bb.useRegs);
                // 任意一个 bb 发生变化则继续迭代
                if (!inAns.equals(bb.inRegs)) {
                    changed = true;
                    bb.inRegs = inAns;
                }
            }
        }
    }
}
