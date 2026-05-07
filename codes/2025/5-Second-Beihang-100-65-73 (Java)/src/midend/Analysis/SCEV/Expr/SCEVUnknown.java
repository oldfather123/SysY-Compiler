package midend.Analysis.SCEV.Expr;

import frontend.ir.Value;

public class SCEVUnknown extends SCEVExpr {
    public Value val;

    public SCEVUnknown(Value val) {
        this.val = val;
    }

    public String toString() {
        return "Unknown(" + val + ")";
    }

    @Override
    public SCEVExpr simplify() {
        return this;
    }

    @Override
    public Value getValueProjected() {
        return val;
    }
}
