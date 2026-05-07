package backend.instruction;

import backend.component.RiscvInstr;
import backend.operand.RiscvImm;
import backend.operand.RiscvReg;

import java.util.ArrayList;
import java.util.Arrays;

public class RiscvSw extends RiscvInstr {
    public RiscvSw(RiscvReg defReg, RiscvImm imm, RiscvReg srcReg) {
        super(null, new ArrayList<>(Arrays.asList(defReg, imm, srcReg)));
    }

    @Override
    public String toString() {
        return "sw" + " " + operands.get(0) + "," + operands.get(1) + "(" + operands.get(2) + ")";
    }
}
