package midend.Analysis.SCEV.Expr;

import frontend.ir.Value;
import frontend.ir.constant.IntConst;

public class SCEVConstant extends SCEVExpr {
    public int value;
    public Value valueProjected;

    public SCEVConstant(int value) {
        this.value = value;
        this.valueProjected = new IntConst(value);
    }

    public int getValue() {
        return value;
    }

    public String toString() {
        return Integer.toString(value);
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

