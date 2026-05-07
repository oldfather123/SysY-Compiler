package cn.edu.bit.newnewcc.frontend.util;

import cn.edu.bit.newnewcc.frontend.Operator;
import cn.edu.bit.newnewcc.ir.Type;
import cn.edu.bit.newnewcc.ir.type.ArrayType;
import cn.edu.bit.newnewcc.ir.type.FloatType;
import cn.edu.bit.newnewcc.ir.type.IntegerType;
import cn.edu.bit.newnewcc.ir.value.Constant;
import cn.edu.bit.newnewcc.ir.value.constant.ConstArray;
import cn.edu.bit.newnewcc.ir.value.constant.ConstFloat;
import cn.edu.bit.newnewcc.ir.value.constant.ConstInt;

import java.util.List;

public final class Constants {
    private Constants() {
    }

    public static Constant convertType(Constant constant, Type targetType) {
        if (constant.getType() == IntegerType.getI32() && targetType == FloatType.getFloat()) {
            int value = ((ConstInt) constant).getValue();
            return ConstFloat.getInstance((float) value);
        }
        if (constant.getType() == FloatType.getFloat() && targetType == IntegerType.getI32()) {
            float value = ((ConstFloat) constant).getValue();
            return ConstInt.getInstance((int) value);
        }
        throw new IllegalArgumentException();
    }

    public static Constant applyUnaryOperator(Constant operand, Operator operator) {
        if (operand.getType() == IntegerType.getI32()) {
            int value = ((ConstInt) operand).getValue();

            if (operator == Operator.LNOT)
                return ConstInt.getInstance(value == 0 ? 1 : 0);

            return ConstInt.getInstance(switch (operator) {
                case POS -> value;
                case NEG -> -value;
                default -> throw new IllegalArgumentException();
            });
        }

        if (operand.getType() == FloatType.getFloat()) {
            float value = ((ConstFloat) operand).getValue();

            if (operator == Operator.LNOT)
                return ConstInt.getInstance(value == 0 ? 1 : 0);

            return ConstFloat.getInstance(switch (operator) {
                case POS -> value;
                case NEG -> -value;
                default -> throw new IllegalArgumentException();
            });
        }

        throw new IllegalArgumentException();
    }

    public static Constant applyBinaryOperator(Constant leftOperand, Constant rightOperand, Operator operator) {
        Type operandType = Types.getCommonType(leftOperand.getType(), rightOperand.getType());

        if (!leftOperand.getType().equals(operandType))
            leftOperand = convertType(leftOperand, operandType);
        if (!rightOperand.getType().equals(operandType))
            rightOperand = convertType(rightOperand, operandType);

        if (operandType == IntegerType.getI32()) {
            int leftValue = ((ConstInt) leftOperand).getValue();
            int rightValue = ((ConstInt) rightOperand).getValue();

            return ConstInt.getInstance(switch (operator) {
                case ADD -> leftValue + rightValue;
                case SUB -> leftValue - rightValue;
                case MUL -> leftValue * rightValue;
                case DIV -> rightValue == 0 ? 0 : leftValue / rightValue;
                case MOD -> rightValue == 0 ? 0 : leftValue % rightValue;
                default -> throw new IllegalArgumentException();
            });
        }

        if (operandType == FloatType.getFloat()) {
            float leftValue = ((ConstFloat) leftOperand).getValue();
            float rightValue = ((ConstFloat) rightOperand).getValue();

            return ConstFloat.getInstance(switch (operator) {
                case ADD -> leftValue + rightValue;
                case SUB -> leftValue - rightValue;
                case MUL -> leftValue * rightValue;
                case DIV -> leftValue / rightValue;
                case MOD -> leftValue % rightValue;
                default -> throw new IllegalArgumentException();
            });
        }

        throw new IllegalArgumentException();
    }
}
