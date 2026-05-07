package riscv.util;

import riscv.value.AsmInstr;
import riscv.value.AsmModule;
import riscv.value.AsmReg;
import util.nodelist.NodeList;

import static riscv.value.AsmInstr.*;

// 这一个 pass 将所有不符合立即数范围的指令转换
public class JustifyShortImm {
    public static void runJustify(AsmModule module) {
        run(module.globalInstrs);
        for (var func : module.funcMap.values()) {
            for (var bb : func.bbList) {
                run(bb.instrs);
            }
        }
    }

    private static void run(NodeList<AsmInstr> instrs) {
        for (var p : instrs) {
            AsmInstr instr = p.get();
            if (instr instanceof IType iType) {
                if (!isI12(iType.imm)) {
                    int imm = iType.imm;
                    AsmReg rd = iType.rd;
                    AsmReg rs = iType.rs;
                    p.insertBefore(new Li(AsmReg.rtmp, imm));
                    if (iType instanceof Addi) {
                        p.set(new Add(rd, rs, AsmReg.rtmp));
                    }
                    else if (iType instanceof Andi) {
                        p.set(new And(rd, rs, AsmReg.rtmp));
                    }
                    else if (iType instanceof Ori) {
                        p.set(new Or(rd, rs, AsmReg.rtmp));
                    }
                    else if (iType instanceof Xori) {
                        p.set(new Xor(rd, rs, AsmReg.rtmp));
                    }
                    else if (iType instanceof Slti) {
                        p.set(new Slt(rd, rs, AsmReg.rtmp));
                    }
                    else if (iType instanceof Sltiu) {
                        p.set(new Sltu(rd, rs, AsmReg.rtmp));
                    }
                    // shifts, not likely
                    else if (iType instanceof Slli) {
                        p.set(new Sll(rd, rs, AsmReg.rtmp));
                    }
                    else if (iType instanceof Srli) {
                        p.set(new Srl(rd, rs, AsmReg.rtmp));
                    }
                    else if (iType instanceof Srai) {
                        p.set(new Sra(rd, rs, AsmReg.rtmp));
                    }
                    else throw new AssertionError(iType);
                }
            }
            else if (instr instanceof StoreType store) {
                if (!isI12(store.offset)) {
                    p.insertBefore(new Li(AsmReg.rtmp, store.offset));
                    p.insertBefore(new Add(AsmReg.rtmp, store.rbase, AsmReg.rtmp));
                    store.offset = 0;
                    store.rbase = AsmReg.rtmp;
                }
            }
            else if (instr instanceof LoadType load) {
                if (!isI12(load.offset)) {
                    p.insertBefore(new Li(AsmReg.rtmp, load.offset));
                    p.insertBefore(new Add(AsmReg.rtmp, load.rbase, AsmReg.rtmp));
                    load.offset = 0;
                    load.rbase = AsmReg.rtmp;
                }
            }
        }
    }

    // 短立即数只支持 12 位
    public static boolean isI12(int x) {
        return -2048 <= x && x <= 2047;
    }
}
