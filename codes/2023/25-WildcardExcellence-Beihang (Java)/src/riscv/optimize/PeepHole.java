package riscv.optimize;

import riscv.value.AsmInstr;
import riscv.value.AsmModule;
import riscv.value.AsmReg;
import util.nodelist.Node;
import util.nodelist.NodeList;

public class PeepHole {
    public static void optimize(AsmModule module) {
        run(module.globalInstrs);
        for (var func : module.funcMap.values()) {
            for (var bb : func.bbList) {
                run(bb.instrs);
            }
        }
    }

    private static void run(NodeList<AsmInstr> instrs) {
        for (var p : instrs) {
            convertToMove(p);
            deleteRedundantMove(p);
            if (instrs.hasNext(p)) {
                deleteUnnecessaryMemoryAccess(p, p.next());
            }
        }
    }

    private static void convertToMove(Node<AsmInstr> p) {
        AsmReg rd = null, rs = null;
        boolean doConvert = false;
        if (p.get() instanceof AsmInstr.Add add) {
            if (add.rs1 == AsmReg.zero) {
                rd = add.rd;
                rs = add.rs2;
                doConvert = true;
            }
            else if (add.rs2 == AsmReg.zero) {
                rd = add.rd;
                rs = add.rs1;
                doConvert = true;
            }
        }
        else if (p.get() instanceof AsmInstr.Addi addi) {
            if (addi.imm == 0) {
                rd = addi.rd;
                rs = addi.rs;
                doConvert = true;
            }
        }
        else if (p.get() instanceof AsmInstr.Sub sub) {
            if (sub.rs2 == AsmReg.zero) {
                rd = sub.rd;
                rs = sub.rs1;
                doConvert = true;
            }
        }

        if (doConvert) {
            assert rd != null && rs != null;
            p.set(new AsmInstr.Mv(rd, rs));
        }
    }

    private static void deleteRedundantMove(Node<AsmInstr> p) {
        boolean judge = (p.get() instanceof AsmInstr.Mv mv && mv.rd == mv.rs) ||
                (p.get() instanceof AsmInstr.FMv fmv && fmv.rd == fmv.rs);
        if (judge) {
            p.remove();
        }
    }

    private static void deleteUnnecessaryMemoryAccess(Node<AsmInstr> p1, Node<AsmInstr> p2) {
        // 连续的 sw + lw 或 lw + sw，删除后者
        boolean judge1 = p1.get() instanceof AsmInstr.Sw sw &&
                p2.get() instanceof AsmInstr.Lw lw &&
                sw.rs == lw.rd &&
                sw.rbase == lw.rbase &&
                sw.offset == lw.offset;
        boolean judge2 = p1.get() instanceof AsmInstr.Lw lw &&
                p2.get() instanceof AsmInstr.Sw sw &&
                sw.rs == lw.rd &&
                sw.rbase == lw.rbase &&
                sw.offset == lw.offset;
        if (judge1 || judge2) p2.remove();
    }
}
