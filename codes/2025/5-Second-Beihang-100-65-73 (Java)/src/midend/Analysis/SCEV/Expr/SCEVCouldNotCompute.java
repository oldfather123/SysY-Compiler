package midend.Analysis.SCEV.Expr;

import frontend.ir.Value;

public class SCEVCouldNotCompute extends SCEVExpr {
    private static final SCEVCouldNotCompute instance = new SCEVCouldNotCompute();

    private SCEVCouldNotCompute() {}

    public static SCEVCouldNotCompute get() {
        return instance;
    }

    @Override
    public String toString() {
        return "CouldNotCompute";
    }

    @Override
    public SCEVExpr simplify() {
        return this;
    }

    @Override
    public Value getValueProjected() {
        return null;
    }
}