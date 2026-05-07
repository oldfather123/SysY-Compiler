package backend.instruction;

import backend.component.RiscvInstr;
import backend.operand.RiscvOperand;
import backend.operand.RiscvReg;

import java.util.ArrayList;

public class RiscvConvert extends RiscvInstr {

    public final boolean ToFloat;

    public RiscvConvert(ArrayList<RiscvOperand> operands, boolean toFloat, RiscvReg defReg) {
        super(defReg, operands);
        ToFloat = toFloat;
    }

    @Override
    public String toString() {
        if(ToFloat){
            return "fcvt.s.w" + "\t" + defReg + "," + operands.get(0);
        }
        else{
            return "fcvt.w.s" + "\t"+defReg + "," + operands.get(0) + "," + "rtz";
        }
    }
}
