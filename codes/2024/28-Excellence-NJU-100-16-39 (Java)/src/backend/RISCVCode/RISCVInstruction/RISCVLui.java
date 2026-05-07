package backend.RISCVCode.RISCVInstruction;

import backend.RISCVCode.RISCVOperand;

/**
 * 用于将浮点数前20位加载到一个整数寄存器中
 */
public class RISCVLui extends RISCVInstruction{
    private final RISCVOperand dest;

    private final RISCVOperand operand1;

    public RISCVLui(RISCVOperand dest, RISCVOperand operand1) {
        this.dest = dest;
        this.operand1 = operand1;
    }

    @Override
    public String getString() {
        return "  lui " + dest.getName() + ", " + operand1.getName() + "\n";
    }
}
