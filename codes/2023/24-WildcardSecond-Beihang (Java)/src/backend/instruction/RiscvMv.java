package backend.instruction;

import backend.component.RiscvInstr;
import backend.operand.RiscvReg;

import java.util.ArrayList;
import java.util.Arrays;

public class RiscvMv extends RiscvInstr {
    public RiscvMv(RiscvReg srcReg, RiscvReg defReg) {
        super(defReg, new ArrayList<>(Arrays.asList(srcReg)));
    }

    @Override
    public String toString() {
        return "mv" + " " + defReg + "," + operands.get(0);
    }
}
