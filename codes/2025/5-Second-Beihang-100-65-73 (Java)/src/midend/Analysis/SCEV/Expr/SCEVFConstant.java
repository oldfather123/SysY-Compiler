package midend.Analysis.SCEV.Expr;

import frontend.ir.Value;
import frontend.ir.constant.FloatConst;

public class SCEVFConstant extends SCEVExpr {
    public float value;
    public Value valueProjected;

    public SCEVFConstant(float value) {
        this.value = value;
        this.valueProjected = new FloatConst(value);
    }

    public String toString() {
        return Float.toString(value);
    }

    @Override
    public SCEVExpr simplify() {
        return this;
    }

    @Override
    public Value getValueProjected() {
        return valueProjected;
    }
}

