package backend.RISCVCode;

/**
 * RISCV操作数
 */
public class RISCVOperand {
    private OperandType operandType;

    private String name;

    public RISCVOperand(OperandType operandType, String name) {
        this.operandType = operandType;
        this.name = name;
    }

    public OperandType getOperandType() {
        return operandType;
    }

    public String getName() {
        return name;
    }
}
