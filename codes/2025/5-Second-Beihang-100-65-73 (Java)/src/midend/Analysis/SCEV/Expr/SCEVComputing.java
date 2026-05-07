package midend.Analysis.SCEV.Expr;

import frontend.ir.Value;

import java.util.HashSet;

public class SCEVComputing extends SCEVExpr {
    private final HashSet<SCEVExpr> users = new HashSet<>();
    private final Value valueProjected;

    public SCEVComputing(Value valueProjected) {
        this.valueProjected = valueProjected;
    }

    @Override
    public String toString() {
        return "Computing";
    }

    @Override
    public SCEVExpr simplify() {
        return this;
    }

    @Override
    public Value getValueProjected() {
        return valueProjected;
    }

    public void addUser(SCEVExpr user) {
        users.add(user);
    }

    public void clearUse() {
        for (SCEVExpr user : users) {
            user.replaceComputing(this, user);
        }
    }
}