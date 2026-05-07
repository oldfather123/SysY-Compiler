package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.type.IntegerType;
import cn.edu.bit.newnewcc.ir.value.Instruction;

import java.util.ArrayList;
import java.util.List;

/**
 * 对于两个整数，求其中的较小值
 */
public class SignedMaxInst extends Instruction {
    protected final Operand operand1, operand2;

    /**
     * @param type 语句返回值的类型
     */
    public SignedMaxInst(IntegerType type) {
        this(type, null, null);
    }

    public SignedMaxInst(IntegerType type, Value value1, Value value2) {
        super(type);
        this.operand1 = new Operand(this, type, value1);
        this.operand2 = new Operand(this, type, value2);
    }

    @Override
    public IntegerType getType() {
        return (IntegerType) super.getType();
    }

    @Override
    public void emitIr(StringBuilder builder) {
        builder.append(String.format(
                "%s = call %s @llvm.smax.%s(%s %s, %s %s)",
                getValueNameIR(),
                getTypeName(),
                getTypeName(),
                getTypeName(),
                getOperand1().getValueNameIR(),
                getTypeName(),
                getOperand2().getValueNameIR()
        ));
    }

    public Value getOperand1() {
        return operand1.getValue();
    }

    public Value getOperand2() {
        return operand2.getValue();
    }

    @Override
    public List<Operand> getOperandList() {
        var list = new ArrayList<Operand>();
        list.add(operand1);
        list.add(operand2);
        return list;
    }
}
