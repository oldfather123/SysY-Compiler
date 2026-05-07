package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Operand;
import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.type.FloatType;
import cn.edu.bit.newnewcc.ir.value.Instruction;

import java.util.ArrayList;
import java.util.List;

/**
 * 浮点数取相反数指令
 */
public class FloatNegateInst extends Instruction {
    private final Operand operand1;

    public FloatNegateInst(FloatType type){
        this(type,null);
    }

    public FloatNegateInst(FloatType type, Value operand1) {
        super(type);
        this.operand1 = new Operand(this,type,operand1);
    }

    public Value getOperand1() {
        return operand1.getValue();
    }

    public void setOperand1(Value value) {
        operand1.setValue(value);
    }

    @Override
    public FloatType getType() {
        return (FloatType) super.getType();
    }

    @Override
    public void emitIr(StringBuilder builder) {
        builder.append(String.format(
                "%s = fneg %s %s",
                getValueNameIR(),
                getTypeName(),
                getOperand1().getValueNameIR()
        ));
    }

    @Override
    public List<Operand> getOperandList() {
        var list = new ArrayList<Operand>();
        list.add(operand1);
        return list;
    }
}
