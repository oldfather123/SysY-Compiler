package backend.instruction;

import backend.component.RiscvInstr;
import backend.operand.RiscvImm;
import backend.operand.RiscvReg;

import java.util.ArrayList;
import java.util.Arrays;

public class RiscvLui extends RiscvInstr {

    public RiscvLui(RiscvImm imm, RiscvReg defReg) {
        super(defReg, new ArrayList<>(Arrays.asList(imm)));
    }

    @Override
    public String toString() {
        return "lui " + defReg + "," + this.operands.get(0);
    }
}
