package midend.Analysis.SCEV.Expr;

import frontend.ir.Value;
import midend.Analysis.LoopInfo;
import midend.Analysis.SCEV.SCEVManager;

import java.util.List;

public class SCEVAddRecExpr extends SCEVExpr {
    public Value valueProjected;
    public final List<SCEVExpr> operands; // [init, step1, step2, ...]
    public final LoopInfo loop;

    public SCEVAddRecExpr(List<SCEVExpr> ops, LoopInfo loop, Value value) {
        this.operands = ops;
        this.loop = loop;
        this.valueProjected = value;
    }

    @Override
    public String toString() {
        return "{" + String.join(", +, ", operands.stream().map(Object::toString).toList()) + "}<loop" + loop.getCounter() + ">";
    }

    @Override
    public SCEVExpr simplify() {
        if (SCEVManager.simplifying.contains(this)) {
            return this;
        }
        SCEVManager.simplifying.add(this);
        operands.replaceAll(SCEVExpr::simplify);
        SCEVManager.simplifying.remove(this);
        return this;
    }

    @Override
    public void replaceComputing(SCEVExpr computing, SCEVExpr replacing) {
        for (int i = 0; i < operands.size(); i++) {
            if (operands.get(i) == computing) {
                operands.set(i, replacing);
            }
        }
    }

    public boolean inTheSameLoop() {
        LoopInfo loop = null;
        for (SCEVExpr operand : operands) {
            if (operand instanceof SCEVConstant) continue;
            if (!(operand instanceof SCEVAddRecExpr addRecExpr)) return false;
            if (loop == null) {
                loop = addRecExpr.loop;
            } else if (!loop.equals(addRecExpr.loop)) {
                return false; // 不在同一个循环
            }
        }
        return loop == null || loop.equals(this.loop); // 如果没有归纳变量，或者都在同一个循环中
    }

    @Override
    public Value getValueProjected() {
        return valueProjected;
    }
}
