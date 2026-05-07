package Backend.instruction;

import Backend.operand.AsmImm;
import Backend.operand.*;
import Backend.component.AsmInstr;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class AsmMove extends AsmInstr {
    private final ArrayList<AsmOperand> operands = new ArrayList<>(Collections.nCopies(2, null));
    private boolean needChange = false;

    public AsmMove(List<AsmOperand> operands) {
        setDst(operands.get(0));
        setSrc(operands.get(1));
    }

    public void setDst(AsmOperand dst) {
        setDefReg(operands.get(0), dst);
        operands.set(0, dst);
    }

    public void setSrc(AsmOperand src) {
        addUseReg(operands.get(1), src);
        operands.set(1, src);
    }

    public boolean getNeedChange() {
        return needChange;
    }

    public void setNeedChange() {
        needChange = true;
    }

    public AsmOperand getDst() {
        return operands.get(0);
    }

    public AsmOperand getSrc() {
        return operands.get(1);
    }

    @Override
    public void replaceReg(AsmOperand oldReg, AsmOperand newReg) {
        if (operands.get(0).equals(oldReg)) {
            setDst(newReg);
        }
        if (operands.get(1).equals(oldReg)) {
            setSrc(newReg);
        }
    }

    @Override
    public String toString() {
        StringBuilder output = new StringBuilder();
        if (operands.get(1) instanceof FPhysicalReg || operands.get(1) instanceof FVirtualReg) {
            output.append("fmv.s\t");
        } else if (operands.get(1) instanceof AsmImm) {
            output.append("li\t");
        } else {
            output.append("move\t");
        }
        output.append(operands.get(0)).append(",\t").append(operands.get(1));
        return output.toString();
    }
}
