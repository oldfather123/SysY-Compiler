package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.exception.IllegalArgumentException;
import cn.edu.bit.newnewcc.ir.type.FloatType;
import cn.edu.bit.newnewcc.ir.type.IntegerType;
import cn.edu.bit.newnewcc.ir.value.Instruction;

import java.util.ArrayList;
import java.util.List;

/**
 * 有符号整数转浮点数指令
 *
 * @see <a href="https://llvm.org/docs/LangRef.html#sitofp-to-instruction">LLVM IR文档</a>
 */
public class SignedIntegerToFloatInst extends Instruction {

    private final Operand sourceOperand;

    public SignedIntegerToFloatInst(IntegerType sourceType, FloatType targetType) {
        super(targetType);
        this.sourceOperand = new Operand(this, sourceType, null);
    }

    public SignedIntegerToFloatInst(Value sourceValue, FloatType targetType) {
        super(targetType);
        if (!(sourceValue.getType() instanceof IntegerType)) {
            throw new IllegalArgumentException("Source value must be of integer type");
        }
        this.sourceOperand = new Operand(this, sourceValue.getType(), sourceValue);
    }

    public Value getSourceOperand() {
        return sourceOperand.getValue();
    }

    public void setSourceOperand(Value value) {
        sourceOperand.setValue(value);
    }

    public IntegerType getSourceType() {
        return (IntegerType) sourceOperand.getType();
    }

    public FloatType getTargetType() {
        return getType();
    }

    public FloatType getType() {
        return (FloatType) super.getType();
    }

    @Override
    public List<Operand> getOperandList() {
        var list = new ArrayList<Operand>();
        list.add(sourceOperand);
        return list;
    }

    @Override
    public void emitIr(StringBuilder builder) {
        builder.append(String.format(
                "%s = sitofp %s %s to %s",
                this.getValueNameIR(),
                getSourceOperand().getTypeName(),
                getSourceOperand().getValueNameIR(),
                this.getTypeName()
        ));
    }
}
