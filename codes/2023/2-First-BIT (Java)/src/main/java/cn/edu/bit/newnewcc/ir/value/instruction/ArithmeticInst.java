package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Type;

public abstract class ArithmeticInst extends BinaryInstruction {
    public ArithmeticInst(Type operandType) {
        super(operandType, operandType, operandType);
    }

    @Override
    public void emitIr(StringBuilder builder) {
        builder.append(String.format(
                "%s = %s %s %s, %s",
                this.getValueNameIR(),
                this.getInstName(),
                this.getTypeName(),
                getOperand1().getValueNameIR(),
                getOperand2().getValueNameIR()
        ));
    }
}
