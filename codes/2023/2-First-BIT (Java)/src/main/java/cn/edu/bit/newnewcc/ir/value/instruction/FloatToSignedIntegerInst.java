package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.type.FloatType;
import cn.edu.bit.newnewcc.ir.type.IntegerType;
import cn.edu.bit.newnewcc.ir.value.Instruction;

import java.util.ArrayList;
import java.util.List;

/**
 * 浮点数转有符号整数指令
 * @see <a href="https://llvm.org/docs/LangRef.html#fptosi-to-instruction">LLVM IR文档</a>
 */
public class FloatToSignedIntegerInst extends Instruction {
    private final Operand sourceOperand;

    public FloatToSignedIntegerInst(FloatType sourceType, IntegerType targetType){
        super(targetType);
        this.sourceOperand = new Operand(this,sourceType,null);
    }

    public FloatToSignedIntegerInst(Value sourceValue, IntegerType targetType){
        super(targetType);
        if(!(sourceValue.getType() instanceof FloatType)){
            throw new IllegalArgumentException("Source value must be of floating point type");
        }
        this.sourceOperand = new Operand(this, sourceValue.getType(), sourceValue);
    }

    public Value getSourceOperand() {
        return sourceOperand.getValue();
    }

    public void setSourceOperand(Value value) {
        sourceOperand.setValue(value);
    }

    public FloatType getSourceType() {
        return (FloatType) sourceOperand.getType();
    }

    public IntegerType getTargetType() {
        return getType();
    }

    @Override
    public IntegerType getType() {
        return (IntegerType) super.getType();
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
                "%s = fptosi %s %s to %s",
                this.getValueNameIR(),
                getSourceOperand().getTypeName(),
                getSourceOperand().getValueNameIR(),
                this.getTypeName()
        ));
    }
}
