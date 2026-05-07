package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.type.IntegerType;

/**
 * 整数运算语句
 * <p>
 * 此类没有存在的必要，只是为了共享整数运算语句通用的代码
 */
public abstract class IntegerArithmeticInst extends ArithmeticInst {

    /**
     * @param type 语句的返回类型，必须是IntegerType
     */
    public IntegerArithmeticInst(IntegerType type) {
        this(type, null, null);
    }

    /**
     * @param operandType 语句的返回类型，必须是IntegerType
     * @param operand1    操作数1
     * @param operand2    操作数2
     */
    public IntegerArithmeticInst(IntegerType operandType, Value operand1, Value operand2) {
        super(operandType);
        setOperand1(operand1);
        setOperand2(operand2);
    }

    @Override
    public IntegerType getType() {
        return (IntegerType) super.getType();
    }

}
