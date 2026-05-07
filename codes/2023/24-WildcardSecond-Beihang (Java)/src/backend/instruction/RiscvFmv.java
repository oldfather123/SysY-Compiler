package backend.instruction;

import backend.component.RiscvInstr;
import backend.operand.RiscvReg;

import java.util.ArrayList;
import java.util.Arrays;

public class RiscvFmv extends RiscvMv {
    public RiscvFmv(RiscvReg srcReg, RiscvReg defReg) {
        super(srcReg, defReg);
    }

    @Override
    public String toString() {
        return "fmv.s" + " " + defReg + "," + operands.get(0);
    }
}
