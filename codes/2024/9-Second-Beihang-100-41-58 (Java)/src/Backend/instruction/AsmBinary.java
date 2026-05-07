package Backend.instruction;

import Backend.operand.AsmImm;
import Backend.operand.AsmOperand;
import Backend.component.AsmInstr;

import java.util.List;
import java.util.ArrayList;
import java.util.Collections;

public class AsmBinary extends AsmInstr {
    private String type;
    private final ArrayList<AsmOperand> operands = new ArrayList<>(Collections.nCopies(3, null));

    public void setDst(AsmOperand dst) {
        setDefReg(operands.get(0), dst);
        operands.set(0, dst);
    }

    public void setSrcLeft(AsmOperand src1) {
        addUseReg(operands.get(1), src1);
        operands.set(1, src1);
    }

    public void setSrcRight(AsmOperand src2) {
        addUseReg(operands.get(2), src2);
        operands.set(2, src2);
    }


    public AsmBinary(String type, List<AsmOperand> operands) {
        this.type = type;
        setDst(operands.get(0));
        setSrcLeft(operands.get(1));
        setSrcRight(operands.get(2));
    }

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public AsmOperand getDst() {
        return operands.get(0);
    }

    public AsmOperand getSrc1() {
        return operands.get(1);
    }

    public AsmOperand getSrc2() {
        return operands.get(2);
    }

    public boolean isSrc2Imm12() {
        return operands.get(2) instanceof AsmImm && ((AsmImm) operands.get(2)).Is12();
    }

    @Override
    public void replaceReg(AsmOperand oldReg, AsmOperand newReg) {
        if (operands.get(0).equals(oldReg)) {
            setDst(newReg);
        }
        if (operands.get(1).equals(oldReg)) {
            setSrcLeft(newReg);
        }
        if (operands.get(2).equals(oldReg)) {
            setSrcRight(newReg);
        }
    }

    @Override
    public String toString() {
        StringBuilder output = new StringBuilder();
        output.append(type);
        if (isSrc2Imm12() && isImm12Supported(type)) {
            output.append("i");
        }
        output.append("\t").append(operands.get(0)).append(",\t").append(operands.get(1)).append(",\t").append(operands.get(2));
        return output.toString();
    }

    private boolean isImm12Supported(String type) {
        return type.equals("add") || type.equals("xor") || type.equals("or") || type.equals("and") ||
                type.equals("sll") || type.equals("srl") || type.equals("sra");
    }

}

