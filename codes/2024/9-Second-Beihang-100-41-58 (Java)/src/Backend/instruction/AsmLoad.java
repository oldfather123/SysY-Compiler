package Backend.instruction;

import Backend.operand.FVirtualReg;
import Backend.operand.AsmOperand;
import Backend.component.AsmInstr;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class AsmLoad extends AsmInstr {
    private final String type;
    private final ArrayList<AsmOperand> operands = new ArrayList<>(Collections.nCopies(3, null));

    public AsmLoad(List<AsmOperand> operands, String type) {
        this.type = type;
        setDst(operands.get(0));
        setAddress(operands.get(1));
        setOffset(operands.get(2));
    }

    public void setDst(AsmOperand dst) {
        setDefReg(operands.get(0), dst);
        operands.set(0, dst);
    }

    public void setAddress(AsmOperand address) {
        addUseReg(operands.get(1), address);
        operands.set(1, address);
    }

    public void setOffset(AsmOperand offset) {
        addUseReg(operands.get(2), offset);
        operands.set(2, offset);
    }

    public AsmOperand getAddress() {
        return operands.get(1);
    }

    public AsmOperand getDst() {
        return operands.get(0);
    }

    public AsmOperand getOffset() {
        return operands.get(2);
    }

    @Override
    public void replaceReg(AsmOperand oldReg, AsmOperand newReg) {
        if (operands.get(0).equals(oldReg)) {
            setDst(newReg);
        }
        if (operands.get(1).equals(oldReg)) {
            setAddress(newReg);
        }
        if (operands.get(2) != null && operands.get(2).equals(oldReg)) {
            setOffset(newReg);
        }
    }

    @Override
    public String toString() {
        StringBuilder output = new StringBuilder();
        output.append(type).append("\t").append(operands.get(0));
        output.append(",\t").append(operands.get(2)).append("(").append(operands.get(1)).append(")");
        if (operands.get(0) instanceof FVirtualReg) {
            output.insert(0, "f");  // 在最前面插入"f"前缀
        }
        return output.toString();
    }
}
