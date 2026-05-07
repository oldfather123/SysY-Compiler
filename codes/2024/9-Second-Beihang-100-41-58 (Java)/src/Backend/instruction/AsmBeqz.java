package Backend.instruction;

import Backend.component.AsmBlock;
import Backend.component.AsmInstr;
import Backend.operand.AsmOperand;

import java.util.ArrayList;
import java.util.Collections;

public class AsmBeqz extends AsmInstr {
    private AsmBlock target;
    private final ArrayList<AsmOperand> operands = new ArrayList<>(Collections.nCopies(1, null));

    public AsmBeqz(AsmOperand src, AsmBlock target) {
        setSrc(src);
        this.target = target;
    }

    @Override
    public void replaceReg(AsmOperand oldReg, AsmOperand newReg) {
        if (operands.get(0) != null && operands.get(0).equals(oldReg)) {
            setSrc(newReg);
        }
    }

    public void setSrc(AsmOperand src) {
        addUseReg(operands.get(0), src);
        operands.set(0, src);
    }

    public AsmBlock getTarget() {
        return target;
    }

    public void setTarget(AsmBlock target) {
        this.target = target;
    }

    @Override
    public String toString() {
        return "beqz\t" + operands.get(0) + ",\t" + target.getName();
    }
}
