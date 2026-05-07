package backend.instruction;

import backend.component.RiscvInstr;
import backend.operand.RiscvReg;

import java.util.ArrayList;
import java.util.Arrays;

public class RiscvJr extends RiscvInstr {
    public RiscvJr(RiscvReg reg) {
        super(null, new ArrayList<>(Arrays.asList(reg)));
    }

    @Override
    public String toString() {
        return "jr" + " " + operands.get(0);
    }
}
