package backend.instruction;

import backend.component.RiscvInstr;
import backend.operand.RiscvImm;
import backend.operand.RiscvLabel;
import backend.operand.RiscvReg;

import java.util.ArrayList;
import java.util.Arrays;

public class RiscvFsw extends RiscvInstr {
    public RiscvFsw(RiscvReg riscvReg, RiscvImm imm, RiscvReg offSet) {
        super(null, new ArrayList<>(Arrays.asList(riscvReg, imm, offSet)));
    }

    @Override
    public String toString() {
        return "fsw" + " " + operands.get(0) + "," + operands.get(1) + "(" + operands.get(2) + ")";
    }
}
