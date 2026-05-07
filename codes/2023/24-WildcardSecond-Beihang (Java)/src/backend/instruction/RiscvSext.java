package backend.instruction;

import backend.component.RiscvInstr;
import backend.operand.RiscvReg;

import java.util.ArrayList;
import java.util.Arrays;

public class RiscvSext extends RiscvInstr {
    public RiscvSext(RiscvReg srcReg, RiscvReg defReg) {
        super(defReg, new ArrayList<>(Arrays.asList(srcReg)));
    }

    @Override
    public String toString() {
        return "sext.w" + " " + defReg + "," + operands.get(0);
    }
}
