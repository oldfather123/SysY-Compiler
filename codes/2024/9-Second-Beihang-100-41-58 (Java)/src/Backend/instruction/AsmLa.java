package Backend.instruction;

import Backend.component.AsmInstr;
import Backend.operand.AsmOperand;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class AsmLa extends AsmInstr {
    private final ArrayList<AsmOperand> operands = new ArrayList<>(Collections.nCopies(2, null));

    public AsmLa(List<AsmOperand> operands) {
        setDst(operands.get(0));
        setAddress(operands.get(1));
    }

    public void setDst(AsmOperand dst) {
        setDefReg(operands.get(0), dst);
        operands.set(0, dst);
    }

    public void setAddress(AsmOperand address) {
        addUseReg(operands.get(1), address);
        operands.set(1, address);
    }

    public void replaceReg(AsmOperand oldReg, AsmOperand newReg) {
        if (operands.get(0).equals(oldReg)) {
            setDst(newReg);
        }
        if (operands.get(1).equals(oldReg)) {
            setAddress(newReg);
        }
    }

    public AsmOperand getDst() {
        return operands.get(0);
    }

    @Override
    public String toString() {
        return "la" + "\t" + operands.get(0) + ",\t" + operands.get(1);
    }
}
