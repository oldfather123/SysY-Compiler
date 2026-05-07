package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.type.FloatType;

/**
 * 浮点数除法语句
 */
public class FloatDivideInst extends FloatArithmeticInst {

    /**
     * @param type 语句的返回类型，必须是FloatType
     * @param operand1 操作数1
     * @param operand2 操作数2
     */
    public FloatDivideInst(FloatType type, Value operand1, Value operand2) {
        super(type, operand1, operand2);
    }

    /**
     * @param type 语句的返回类型，必须是FloatType
     */
    public FloatDivideInst(FloatType type) {
        super(type);
    }

    @Override
    protected String getInstName() {
        return "fdiv";
    }
}
