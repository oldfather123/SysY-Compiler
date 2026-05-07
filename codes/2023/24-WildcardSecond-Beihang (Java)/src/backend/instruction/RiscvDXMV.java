package backend.instruction;

import backend.component.RiscvInstr;
import backend.operand.RiscvFPUReg;
import backend.operand.RiscvReg;

import java.util.ArrayList;
import java.util.Arrays;

public class RiscvDXMV extends RiscvInstr {
    public RiscvDXMV(RiscvReg srcReg, RiscvReg defReg) {
        super(defReg, new ArrayList<>(Arrays.asList(srcReg)));
    }

    @Override
    public String toString() {
        if(defReg instanceof RiscvFPUReg){
            return "fmv.d.x" + " " + defReg + "," + operands.get(0);
        }else{
            return "fmv.x.d" + " " + defReg + "," + operands.get(0);
        }
    }
}
