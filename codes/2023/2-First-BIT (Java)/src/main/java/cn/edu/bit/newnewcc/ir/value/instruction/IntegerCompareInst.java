package cn.edu.bit.newnewcc.ir.value.instruction;

import cn.edu.bit.newnewcc.ir.Value;
import cn.edu.bit.newnewcc.ir.type.IntegerType;

/**
 * 整数比较语句
 */
public class IntegerCompareInst extends CompareInst {

    public enum Condition {
        EQ, NE, SLT, SLE, SGT, SGE;

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
                case EQ -> EQ;
                case NE -> NE;
                case SLT -> SGT;
                case SLE -> SGE;
                case SGT -> SLT;
                case SGE -> SLE;
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
                case EQ -> NE;
                case NE -> EQ;
                case SLT -> SGE;
                case SLE -> SGT;
                case SGT -> SLE;
                case SGE -> SLT;
            };
        }
    }

    private final Condition condition;

    /**
     * @param comparedType 待比较数据的类型，必须是IntegerType
     * @param condition    比较的方法
     */
    public IntegerCompareInst(IntegerType comparedType, Condition condition) {
        this(comparedType, condition, null, null);
    }

    /**
     * @param comparedType 待比较数据的类型，必须是IntegerType
     * @param condition    比较的方法
     * @param operand1     操作数1
     * @param operand2     操作数2
     */
    public IntegerCompareInst(IntegerType comparedType, Condition condition, Value operand1, Value operand2) {
        super(comparedType);
        this.condition = condition;
        setOperand1(operand1);
        setOperand2(operand2);
    }

    public Condition getCondition() {
        return condition;
    }

    @Override
    public IntegerType getComparedType() {
        return (IntegerType) super.getComparedType();
    }

    @Override
    public IntegerType getType() {
        return (IntegerType) super.getType();
    }

    @Override
    protected String getInstName() {
        return "icmp " + switch (condition) {
            case EQ -> "eq";
            case NE -> "ne";
            case SLT -> "slt";
            case SLE -> "sle";
            case SGT -> "sgt";
            case SGE -> "sge";
        };
    }

}
