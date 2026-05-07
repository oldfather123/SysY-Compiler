package backend.instruction;

import backend.component.RiscvInstr;
import backend.operand.RiscvImm;
import backend.operand.RiscvReg;

import java.util.ArrayList;
import java.util.Arrays;

public class RiscvLd extends RiscvInstr {
    public RiscvLd(RiscvImm imm, RiscvReg srcReg, RiscvReg defReg) {
        super(defReg, new ArrayList<>(Arrays.asList(imm, srcReg)));
    }

    @Override
    public String toString() {
        return "ld" + " " + defReg + "," + operands.get(0) + "(" + operands.get(1) + ")";
    }
}
