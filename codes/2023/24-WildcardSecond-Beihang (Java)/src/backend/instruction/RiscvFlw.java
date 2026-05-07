package backend.instruction;

import backend.component.RiscvInstr;
import backend.operand.RiscvImm;
import backend.operand.RiscvLabel;
import backend.operand.RiscvReg;

import java.util.ArrayList;
import java.util.Arrays;

public class RiscvFlw extends RiscvInstr {

    public RiscvFlw(RiscvReg riscvReg, RiscvImm imm, RiscvReg offSet) {
        super(riscvReg, new ArrayList<>(Arrays.asList(imm, offSet)));
    }

    @Override
    public String toString() {
        return "flw" + " " + defReg + "," + operands.get(0) + "(" + operands.get(1) + ")";
    }
}
