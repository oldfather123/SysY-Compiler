package riscv.optimize;

import riscv.value.AsmInstr;
import riscv.value.AsmReg;
import util.Pair;

import java.util.ArrayList;
import java.util.List;

import static java.lang.Math.abs;
import static riscv.value.AsmReg.rimm;
import static riscv.value.AsmReg.rtmp;

public class Divi {
    private static final boolean noOpt = false;

    public static List<AsmInstr> genDivi(AsmReg rd, AsmReg rs, int imm) {
        List<AsmInstr> result = new ArrayList<>(3);

        if (noOpt) {
            result.add(new AsmInstr.Li(rimm, imm));
            result.add(new AsmInstr.Div(rd, rs, rimm));
            return result;
        }

        int l = ceilLog2(abs(imm));

        if (imm == 0) {
            System.err.println("Divided by zero!");
            result.add(new AsmInstr.Li(rd, 0));
        }
        else if (imm == 1) {
            result.add(new AsmInstr.Mv(rd, rs));
        }
        else if (imm == -1) {
            result.add(AsmInstr.Pseudo.Neg(rd, rs));
        }
        else if (imm == -2147483648) {
            // n == 0x80000000 ? 1 : 0;
            result.add(new AsmInstr.Xori(rd, rs, 0x80000000).withComment("div " + imm));
            result.add(AsmInstr.Pseudo.Seqz(rd, rd));
        }
        else if (abs(imm) == (1L << l)) {
            // ans = sra(n + srl(sra(n, l - 1), 32 - l), l);
            // 考虑到可能 rd 和 rs 相同，写 rd 之后，不允许再读 rs
            result.add(new AsmInstr.Srai(rtmp, rs, l - 1).withComment("div " + imm));
            result.add(new AsmInstr.Srli(rtmp, rtmp, 32 - l));
            result.add(new AsmInstr.Add(rtmp, rtmp, rs));
            result.add(new AsmInstr.Srai(rd, rtmp, l));
            if (imm < 0) result.add(AsmInstr.Pseudo.Neg(rd, rd));
        }
        else {
            // 考虑到可能 rd 和 rs 相同，写 rd 之后，不允许再读 rs
            Pair<Long, Integer> ms = chooseMultiplier(abs(imm), 31);
            long m = ms.first;
            int s = ms.second;
            // if m < 2 ** 31:   ans = sra(mulsh(m, n), s) - xsign(n);
            // else:             ans = sra(n + mulsh(m - (1L << 32), n), s) - xsign(n);
            if (m < (1L << 31)) {
                mulh32i(result, rtmp, rs, m);
                result.add(new AsmInstr.Srai(rtmp, rtmp, s));
                xsign(result, rd, rs);
                result.add(new AsmInstr.Sub(rd, rtmp, rd));
                result.get(0).comment = "div " + imm;
            }
            else {
                mulh32i(result, rtmp, rs, m - (1L << 32));
                result.add(new AsmInstr.Add(rtmp, rtmp, rs));
                result.add(new AsmInstr.Srai(rtmp, rtmp, s));
                xsign(result, rd, rs);
                result.add(new AsmInstr.Sub(rd, rtmp, rd));
                result.get(0).comment = "div " + imm;
            }
        }

        assert result.size() > 0;
        return result;
    }

    public static List<AsmInstr> genIDivR(AsmReg rd, int imm, AsmReg rs) {
        List<AsmInstr> result = new ArrayList<>(3);
        if (imm == 0) {
            result.add(new AsmInstr.Li(rd, 0));
        }
        else {
            result.add(new AsmInstr.Li(rtmp, imm));
            result.add(new AsmInstr.Div(rd, rtmp, rs));
        }
        return result;
    }

    // 32 位高位立即数乘，Risc 只有 64 位的版本 的 mulh
    private static void mulh32i(List<AsmInstr> instrs, AsmReg rd, AsmReg rs, long imm) {
        instrs.add(new AsmInstr.Li64(rtmp, imm));
        instrs.add(new AsmInstr.Mul(rd, rs, rtmp));
        instrs.add(new AsmInstr.Srli64(rd, rd, 32));
    }

    // 负数返回 -1，否则为 0
    private static void xsign(List<AsmInstr> instrs, AsmReg rd, AsmReg rs) {
        instrs.add(new AsmInstr.Srai(rd, rs, 31));
    }

    // return (m, s)
    private static Pair<Long, Integer> chooseMultiplier(int inst, int bit) {
        assert inst > 0;
        int s = ceilLog2(inst);
        long low = Long.divideUnsigned(1L << (32 + s), inst);
        long high = Long.divideUnsigned((1L << (32 + s)) + (1L << (32 + s - bit)), inst);
        while (Long.compareUnsigned(low >>> 1, high >>> 1) < 0 && s > 0) {
            low >>>= 1;
            high >>>= 1;
            s--;
        }
        return new Pair<>(high, s);
    }

    private static int ceilLog2(int x) {
        return 31 - Integer.numberOfLeadingZeros(x);
    }
}
