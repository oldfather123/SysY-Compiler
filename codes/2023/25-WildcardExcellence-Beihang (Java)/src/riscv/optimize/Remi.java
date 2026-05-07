package riscv.optimize;

import riscv.value.AsmInstr;
import riscv.value.AsmReg;

import java.util.ArrayList;
import java.util.List;

import static riscv.value.AsmReg.rimm;

public class Remi {
    public static List<AsmInstr> genRemi(AsmReg rd, AsmReg rs, int imm) {
        List<AsmInstr> result = new ArrayList<>(3);
        result.add(new AsmInstr.Li(rimm, imm));
        result.add(new AsmInstr.Rem(rd, rs, rimm));
        return result;
    }

    public static List<AsmInstr> genIRemR(AsmReg rd, int imm, AsmReg rs) {
        List<AsmInstr> result = new ArrayList<>(3);
        result.add(new AsmInstr.Li(rimm, imm));
        result.add(new AsmInstr.Rem(rd, rimm, rs));
        return result;
    }
}
