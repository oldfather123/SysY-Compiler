package backend.instruction;

import backend.component.RiscvInstr;
import backend.operand.RiscvImm;
import backend.operand.RiscvReg;

import java.util.ArrayList;
import java.util.Arrays;

public class RiscvFld extends RiscvInstr {
    public RiscvFld(RiscvReg riscvReg, RiscvImm imm, RiscvReg offSet) {
        super(riscvReg, new ArrayList<>(Arrays.asList(imm, offSet)));
    }

    @Override
    public String toString() {
        return "fld" + " " + defReg + "," + operands.get(0) + "(" + operands.get(1) + ")";
    }
}
