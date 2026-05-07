package backend.instruction;

import backend.component.RiscvFunction;
import backend.component.RiscvInstr;
import backend.operand.RiscvCPUReg;
import backend.operand.RiscvLabel;
import backend.operand.RiscvReg;

import java.util.ArrayList;
import java.util.LinkedHashSet;

public class RiscvCall extends RiscvInstr {
    public  RiscvLabel targetFunction;
    public LinkedHashSet<RiscvReg> usedRegs = new LinkedHashSet<RiscvReg>();
    public RiscvCall(RiscvLabel targetFunction) {
        super(RiscvCPUReg.getRiscvCPUReg(1), new ArrayList<>());//修改了ra
        this.targetFunction = targetFunction;
    }
    public void addUsedReg(RiscvReg usedReg) {
        usedRegs.add(usedReg);
    }

    @Override
    public String toString() {
        return "jal " + targetFunction.getName() + "\n";
    }
}
