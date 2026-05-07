package backend.instruction;

import backend.component.RiscvInstr;
import backend.operand.RiscvImm;
import backend.operand.RiscvReg;

import java.util.ArrayList;
import java.util.Arrays;

public class RiscvLi extends RiscvInstr {

    public RiscvLi(RiscvImm imm, RiscvReg defReg) {
        super(defReg, new ArrayList<>(Arrays.asList(imm)));
    }

    @Override
    public String toString() {
        return "li " + defReg + "," + this.operands.get(0);
    }
}
