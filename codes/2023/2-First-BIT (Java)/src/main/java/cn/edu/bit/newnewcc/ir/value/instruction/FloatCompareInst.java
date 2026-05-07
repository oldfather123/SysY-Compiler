package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.type.FloatType;
import cn.edu.bit.newnewcc.ir.type.IntegerType;

/**
 * 浮点数比较语句
 */
public class FloatCompareInst extends CompareInst {

    public enum Condition {
        OEQ, ONE, OLT, OLE, OGT, OGE;

        /**
         * 获取该条件左右交换后的条件
         *
         * @return 交换两侧数值后，与原始条件等价的条件
         */
        public Condition swap() {
            return getSwappedCondition(this);
        }

        /**
         * 获取该条件左右交换后的条件
         *
         * @param condition 原始条件
         * @return 交换两侧数值后，与原始条件等价的条件
         */
        public static Condition getSwappedCondition(Condition condition) {
            return switch (condition) {
                case OEQ -> OEQ;
                case ONE -> ONE;
                case OLT -> OGT;
                case OLE -> OGE;
                case OGT -> OLT;
                case OGE -> OLE;
            };
        }

        /**
         * 获取该条件被 not 作用后的条件
         *
         * @return 该条件被 not 作用后的条件
         */
        public Condition not() {
            return getNotCondition(this);
        }

        /**
         * 获取该条件被 not 作用后的条件
         *
         * @param condition 原始条件
         * @return 该条件被 not 作用后的条件
         */
        public static Condition getNotCondition(Condition condition) {
            return switch (condition) {
                case OEQ -> ONE;
                case ONE -> OEQ;
                case OLT -> OGE;
                case OLE -> OGT;
                case OGT -> OLE;
                case OGE -> OLT;
            };
        }
    }

    private final Condition condition;

    /**
     * @param comparedType 待比较数据的类型，必须是FloatType
     * @param condition    比较的方法
     */
    public FloatCompareInst(FloatType comparedType, Condition condition) {
        this(comparedType, condition, null, null);
    }

    /**
     * @param comparedType 待比较数据的类型，必须是FloatType
     * @param condition    比较的方法
     * @param operand1     操作数1
     * @param operand2     操作数2
     */
    public FloatCompareInst(FloatType comparedType, Condition condition, Value operand1, Value operand2) {
        super(comparedType);
        this.condition = condition;
        setOperand1(operand1);
        setOperand2(operand2);
    }

    public Condition getCondition() {
        return condition;
    }

    @Override
    public FloatType getComparedType() {
        return (FloatType) super.getComparedType();
    }

    @Override
    public IntegerType getType() {
        return (IntegerType) super.getType();
    }

    @Override
    protected String getInstName() {
        return "fcmp " + switch (condition) {
            case OEQ -> "oeq";
            case ONE -> "one";
            case OLT -> "olt";
            case OLE -> "ole";
            case OGT -> "ogt";
            case OGE -> "oge";
        };
    }

}
