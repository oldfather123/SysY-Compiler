package mid.Optimizer.Loop;

import mid.IntermediatePresentation.ConstNumber;
import mid.IntermediatePresentation.User;
import mid.IntermediatePresentation.Value;
import mid.IntermediatePresentation.ValueType;
import mid.Optimizer.Optimizer;

public class ScEvValue extends User {
    private static int num = 0;

    public Value getInitVal() {
        return operandList.get(0);
    }

    public Value getStepVal() {
        return operandList.get(1);
    }

    public ScEvValue(Value initVal, Value stepVal) {
        super("scev" + num, ValueType.NULL);
        num++;
        // 代表scev表达式为 { initVal, +, stepVal }
        use(initVal);
        use(stepVal);
        Optimizer.instance().getLoopAnalyze().getScEvValues().add(this);
    }

    public int hashCode() {
        return toString().hashCode();
    }

    public String toString() {
        return reg + " = {" + getInitVal().getReg() + ", " + getStepVal().getReg() + "}";
    }

    public CmpResult compareTo(ScEvValue scEvValue) {

        CmpResult possibleResult;
        // 首先比较 init
        Value init = getInitVal();
        Value cmpInit = scEvValue.getInitVal();
        if (init instanceof ConstNumber n && cmpInit instanceof ConstNumber cmpN) {
            float nVal = n.getVal().floatValue();
            float cmpNVal = cmpN.getVal().floatValue();
            if (nVal < cmpNVal) {
                possibleResult = CmpResult.LEQ;
            } else if (nVal == cmpNVal) {
                possibleResult = CmpResult.EQ;
            } else {
                possibleResult = CmpResult.GEQ;
            }
        } else {
            if (init.equals(cmpInit)) {
                possibleResult = CmpResult.EQ;
            } else {
                return CmpResult.NCMP;
            }
        }

        Value step = getStepVal();
        Value cmpStep = scEvValue.getStepVal();
        if (step instanceof ConstNumber n && cmpStep instanceof ConstNumber cmpN) {
            float nVal = n.getVal().floatValue();
            float cmpNVal = cmpN.getVal().floatValue();
            if (nVal < cmpNVal) {
                if (possibleResult == CmpResult.GEQ) {
                    return CmpResult.NCMP;
                } else {
                    return CmpResult.LEQ;
                }
            } else if (nVal == cmpNVal) {
                return possibleResult;
            } else {
                if (possibleResult == CmpResult.LEQ) {
                    return CmpResult.NCMP;
                } else {
                    return CmpResult.GEQ;
                }
            }
        } else {
            if (init.equals(cmpInit)) {
                return possibleResult;
            } else {
                return CmpResult.NCMP;
            }
        }
    }

    public enum CmpResult {
        GEQ, EQ, LEQ, NCMP
    }
}
