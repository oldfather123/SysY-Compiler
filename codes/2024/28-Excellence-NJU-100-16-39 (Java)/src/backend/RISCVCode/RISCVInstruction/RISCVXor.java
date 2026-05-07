package backend.RISCVCode.RISCVInstruction;

import backend.RISCVCode.OperandType;
import backend.RISCVCode.RISCVOperand;

public class RISCVXor extends RISCVInstruction{
    private final RISCVOperand dest;

    private final RISCVOperand operand1;

    private final RISCVOperand operand2;


    public RISCVXor(RISCVOperand dest, RISCVOperand operand1, RISCVOperand operand2) {
        this.dest = dest;
        this.operand1 = operand1;
        this.operand2 = operand2;
    }

    @Override
    public String getString() {
        if (operand1.getOperandType() == OperandType.imm || operand2.getOperandType() == OperandType.imm) {
            return "  xori " + dest.getName() + ", " + operand1.getName() + ", " + operand2.getName() + "\n";
        }
        return "  xor " + dest.getName() + ", " + operand1.getName() + ", " + operand2.getName() + "\n";
    }
}
