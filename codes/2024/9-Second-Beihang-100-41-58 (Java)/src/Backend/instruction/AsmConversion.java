package Backend.instruction;

import Backend.operand.AsmOperand;
import Backend.component.AsmInstr;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class AsmConversion extends AsmInstr {
    private final String type;
    private final ArrayList<AsmOperand> operands = new ArrayList<>(Collections.nCopies(2, null));

    public AsmConversion(String type, List<AsmOperand> operands) {
        this.type = type;
        setDst(operands.get(0));
        setSrc(operands.get(1));
    }

    private void setDst(AsmOperand dst) {
        setDefReg(operands.get(0), dst);
        operands.set(0, dst);
    }

    private void setSrc(AsmOperand src) {
        addUseReg(operands.get(1), src);
        operands.set(1, src);
    }

    public String getType() {
        return type;
    }

    public AsmOperand getDst() {
        return operands.get(0);
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
        StringBuilder output = new StringBuilder(type)
                .append("\t")
                .append(operands.get(0))
                .append(",\t")
                .append(operands.get(1));
        if ("fcvt.w.s".equals(type)) {
            output.append(",\trtz");
        }
        return output.toString();
    }
}
