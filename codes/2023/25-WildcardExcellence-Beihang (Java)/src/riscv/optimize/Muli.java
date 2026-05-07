package riscv.optimize;

import riscv.value.AsmInstr;
import riscv.value.AsmReg;

import java.util.ArrayList;
import java.util.List;

import static java.lang.Math.abs;
import static riscv.value.AsmReg.rimm;
import static riscv.value.AsmReg.rtmp;

public class Muli {
    public static List<AsmInstr> genMuli(AsmReg rd, AsmReg rs, int imm) {
        List<AsmInstr> result = new ArrayList<>(3);
        int l = ceilLog2(abs(imm));

        if (imm == 0) {
            result.add(new AsmInstr.Li(rd, 0));
        }
        else if (imm == 1) {
            result.add(new AsmInstr.Mv(rd, rs));
        }
        else if (imm == -1) {
            result.add(AsmInstr.Pseudo.Neg(rd, rs));
        }
        // imm == 2 ** l
        else if (abs(imm) == (1 << l)) {
            result.add(new AsmInstr.Slli(rd, rs, l).withComment("mul " + imm));
            if (imm < 0) result.add(AsmInstr.Pseudo.Neg(rd, rd));
        }
        // imm == 2 ** l + 1
        else if (abs(imm) == (1 << l) + 1) {
            result.add(new AsmInstr.Slli(rtmp, rs, l).withComment("mul " + imm));
            result.add(new AsmInstr.Add(rd, rtmp, rs));
            if (imm < 0) result.add(AsmInstr.Pseudo.Neg(rd, rd));
        }
        // imm == 2 ** (l + 1) - 1
        else if (abs(imm) == (1 << (l + 1)) - 1) {
            result.add(new AsmInstr.Slli(rtmp, rs, l + 1).withComment("mul " + imm));
            result.add(new AsmInstr.Sub(rd, rtmp, rs));
            if (imm < 0) result.add(AsmInstr.Pseudo.Neg(rd, rd));
        }
        // imm == 2 ** l + 2 ** k ?
        else {
            int k = ceilLog2(abs(imm) - (1 << l));
            if (abs(imm) == (1 << l) + (1 << k)) {
                result.add(new AsmInstr.Slli(rtmp, rs, l).withComment("mul " + imm));
                result.add(new AsmInstr.Slli(rd, rs, k));
                result.add(new AsmInstr.Add(rd, rtmp, rd));
                if (imm < 0) result.add(AsmInstr.Pseudo.Neg(rd, rd));
            }
            // no optimize
            else {
                result.add(new AsmInstr.Li(rimm, imm));
                result.add(new AsmInstr.Mul(rd, rs, rimm));
            }
        }
        assert result.size() > 0;
        return result;
    }

    private static int ceilLog2(int x) {
        return 31 - Integer.numberOfLeadingZeros(x);
    }
}
