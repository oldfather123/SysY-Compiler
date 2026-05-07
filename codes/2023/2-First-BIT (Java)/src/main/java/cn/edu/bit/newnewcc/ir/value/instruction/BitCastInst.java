package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.value.Instruction;

import java.util.ArrayList;
import java.util.List;

/**
 * 位转换指令
 * <p>
 * 此指令可以强制将一个类型转化为另一个类型，而不改变内存结构
 * <p>
 * 在本项目中，其唯一用途是调用memset时需要将float*转化为i32*
 *
 * @see <a href="https://llvm.org/docs/LangRef.html#bitcast-to-instruction">LLVM IR文档</a>
 */
public class BitCastInst extends Instruction {
    private final Operand sourceOperand;

    public BitCastInst(Type sourceType, Type targetType) {
        super(targetType);
        this.sourceOperand = new Operand(this, sourceType, null);
    }

    public BitCastInst(Value sourceValue, Type targetType) {
        super(targetType);
        this.sourceOperand = new Operand(this, sourceValue.getType(), sourceValue);
    }

    public Value getSourceOperand() {
        return sourceOperand.getValue();
    }

    public void setSourceOperand(Value value) {
        sourceOperand.setValue(value);
    }

    public Type getSourceType() {
        return sourceOperand.getType();
    }

    public Type getTargetType() {
        return getType();
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
                "%s = bitcast %s %s to %s",
                this.getValueNameIR(),
                getSourceOperand().getTypeName(),
                getSourceOperand().getValueNameIR(),
                this.getTypeName()
        ));
    }
}
