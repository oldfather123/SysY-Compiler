package riscv.optimize;

import riscv.value.AsmInstr;
import riscv.value.AsmModule;
import riscv.value.AsmReg;
import util.nodelist.Node;
import util.nodelist.NodeList;

// 将连续的两个 sw zero 转成一个 sd zero
public class Sw2Sd {
    public static void optimize(AsmModule module) {
        run(module.globalInstrs);
        for (var func : module.funcMap.values()) {
            for (var bb : func.bbList) {
                run(bb.instrs);
            }
        }
    }

    private static void run(NodeList<AsmInstr> instrs) {
        for (var p1 : instrs) {
            if (!instrs.hasNext(p1)) break;
            Node<AsmInstr> p2 = p1.next();

            // sw zero, off(rbase)
            // sw zero, off+4(rbase)
            if (p1.get() instanceof AsmInstr.Sw sw1 &&
                    p2.get() instanceof AsmInstr.Sw sw2 &&
                    sw1.rs == AsmReg.zero &&
                    sw2.rs == AsmReg.zero &&
                    sw1.rbase == sw2.rbase &&
                    (sw2.offset == sw1.offset + 4 || sw2.offset == sw1.offset - 4)
            ) {
                AsmReg rbase = sw1.rbase;
                int offset = Math.min(sw1.offset, sw2.offset);
                p2.remove();
                p1.set(new AsmInstr.Sd(AsmReg.zero, rbase, offset));
            }
        }
    }
}
