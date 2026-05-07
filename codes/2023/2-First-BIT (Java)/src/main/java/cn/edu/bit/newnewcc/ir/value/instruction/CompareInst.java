package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.type.IntegerType;

public abstract class CompareInst extends BinaryInstruction {

    private final Type comparedType;

    public CompareInst(Type comparedType) {
        super(IntegerType.getI1(), comparedType, comparedType);
        this.comparedType = comparedType;
    }

    public Type getComparedType() {
        return comparedType;
    }

    @Override
    public void emitIr(StringBuilder builder) {
        builder.append(String.format(
                "%s = %s %s %s, %s",
                this.getValueNameIR(),
                this.getInstName(),
                this.comparedType.getTypeName(),
                getOperand1().getValueNameIR(),
                getOperand2().getValueNameIR()
        ));
    }
}
