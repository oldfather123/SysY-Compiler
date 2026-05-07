package midend.Analysis.SCEV.Expr;

import frontend.ir.Value;
import frontend.ir.instr.binop.AddInstr;
import midend.Analysis.LoopInfo;
import midend.Analysis.SCEV.SCEVManager;

import java.util.List;

public class SCEVSRemRecExpr extends SCEVExpr {
    public Value valueProjected;
    public final List<SCEVExpr> operands; // [init, step1, step2, ...]
    public final LoopInfo loop;
    public final AddInstr addExpr;

    public SCEVSRemRecExpr(List<SCEVExpr> ops, LoopInfo loop, Value value, AddInstr addExpr) {
        this.operands = ops;
        this.loop = loop;
        this.valueProjected = value;
        this.addExpr = addExpr;
    }

    @Override
    public void replaceComputing(SCEVExpr computing, SCEVExpr replacing) {
        for (int i = 0; i < operands.size(); i++) {
            if (operands.get(i) == computing) {
                operands.set(i, replacing);
            }
        }
    }

    @Override
    public String toString() {
        return "{ SCEVSREMEXPR" + String.join(", +, ", operands.stream().map(Object::toString).toList()) + "}<loop" + loop.getCounter() + ", addExpr" + addExpr + ">";
    }

    @Override
    public SCEVExpr simplify() {
        if(SCEVManager.simplifying.contains(this)) {
            return this;
        }
        SCEVManager.simplifying.add(this);
        operands.replaceAll(SCEVExpr::simplify);
        SCEVManager.simplifying.remove(this);
        return this;
    }

    @Override
    public Value getValueProjected() {
        return valueProjected;
    }
}
