package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.type.IntegerType;

/**
 * 整数左移语句
 */
public class ShiftLeftInst extends IntegerArithmeticInst {

    /**
     * @param type     语句的返回类型，必须是IntegerType
     * @param operand1 操作数1
     * @param operand2 操作数2
     */
    public ShiftLeftInst(IntegerType type, Value operand1, Value operand2) {
        super(type, operand1, operand2);
    }

    /**
     * @param type 语句的返回类型，必须是IntegerType
     */
    public ShiftLeftInst(IntegerType type) {
        super(type);
    }

    @Override
    protected String getInstName() {
        return "shl";
    }

}
