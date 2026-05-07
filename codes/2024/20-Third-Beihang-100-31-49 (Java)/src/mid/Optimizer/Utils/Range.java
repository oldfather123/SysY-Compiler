package mid.Optimizer.Utils;

import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.Instruction.Cmp;
import mid.IntermediatePresentation.Value;

public class Range {
    private Number left;
    private Number right;
    private boolean isLeftAvailable;
    private boolean isRightAvailable;


    public Range() {
        left = Double.NEGATIVE_INFINITY;
        right = Double.POSITIVE_INFINITY;
        isLeftAvailable = false;
        isRightAvailable = false;
    }

    private Range(Number left, Number right, boolean isLeftAvailable, boolean isRightAvailable) {
        this.left = left;
        this.right = right;
        this.isLeftAvailable = isLeftAvailable;
        this.isRightAvailable = isRightAvailable;
    }

    public void addRange(String op, Number value) {
        switch (op) {
            case ">":
                if (value.intValue() > left.intValue() || (value.intValue() == left.intValue() && isLeftAvailable)) {
                    left = value;
                    isLeftAvailable = false;
                }
                break;
            case ">=":
                if (value.intValue() > left.intValue()) {
                    left = value;
                    isLeftAvailable = true;
                }
                break;
            case "<":
                if (value.intValue() < right.intValue() || (value.intValue() == right.intValue() && isRightAvailable)) {
                    right = value;
                    isRightAvailable = false;
                }
                break;
            case "<=":
                if (value.intValue() < right.intValue()) {
                    right = value;
                    isRightAvailable = true;
                }
                break;
            case "==":
                break;
            case "!=":
                break;
            case "+":
                left = left.intValue() + value.intValue();
//                right = right.intValue() + value.intValue();
                break;
            case "-":
                break;
            case "*":
                break;
            case "/":
                break;
            case "%":
                break;
        }
    }

    public int isWithinRange(Cmp cmp) {
        /*
            0: uncertain
            1: true
            2: false
         */
        int ret = 0;
        //左边界大于右边界直接返回
        if (!((left.intValue() < right.intValue()) || (left.intValue() == right.intValue() && isLeftAvailable && isRightAvailable)))
            return ret;
        String cOp = cmp.getOperator();
        Value operand1 = cmp.getOperandList().get(1);
        if (!(operand1 instanceof ConstNumber)) return ret;
        Number cVal = ((ConstNumber) operand1).getVal();
        switch (cOp) {
            case ">":
                if ((left.intValue() == cVal.intValue() && !isLeftAvailable) || left.intValue() > cVal.intValue())
                    ret = 1;
                else if (right.intValue() <= cVal.intValue())
                    ret = 2;
                break;
            case ">=":
                if (left.intValue() >= cVal.intValue())
                    ret = 1;
                else if ((right.intValue() == cVal.intValue() && !isRightAvailable) || right.intValue() < cVal.intValue())
                    ret = 2;
                break;
            case "<":
                if ((right.intValue() == cVal.intValue() && !isRightAvailable) || right.intValue() < cVal.intValue())
                    ret = 1;
                else if (left.intValue() >= cVal.intValue())
                    ret = 2;
                break;
            case "<=":
                if (right.intValue() <= cVal.intValue())
                    ret = 1;
                else if ((left.intValue() == cVal.intValue() && !isLeftAvailable) || left.intValue() > cVal.intValue())
                    ret = 2;
                break;
        }
        return ret;
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        if (isLeftAvailable)
            sb.append("[");
        else
            sb.append("(");
        if (left.equals(Double.POSITIVE_INFINITY))
            sb.append("infinity");
        else
            sb.append(left);
        sb.append(",");
        if (right.equals(Double.NEGATIVE_INFINITY))
            sb.append("infinity");
        else
            sb.append(right);
        if (isRightAvailable)
            sb.append("]");
        else
            sb.append(")");
        return sb.toString();
    }

    public Range deepCopy() {
        return new Range(left, right, isLeftAvailable, isRightAvailable);
    }
}

