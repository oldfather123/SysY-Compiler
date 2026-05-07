package midend.Analysis.SCEV.Expr;

import frontend.ir.Value;
import frontend.ir.instr.binop.SubInstr;
import frontend.ir.structure.BasicBlock;
import midend.Analysis.SCEV.SCEVManager;

import java.util.List;

public class SCEVMinusExpr extends SCEVExpr {
    public Value valueProjected;
    public final List<SCEVExpr> operands;

    public SCEVMinusExpr(List<SCEVExpr> ops, Value valueProjected) {
        this.valueProjected = valueProjected;
        this.operands = ops;
    }

    public String toString() {
        return "(" + String.join(" - ", operands.stream().map(Object::toString).toList()) + ")";
    }

    @Override
    public SCEVExpr simplify() {
        if (SCEVManager.simplifying.contains(this)) {
            return this;
        }
        SCEVManager.simplifying.add(this);
        operands.replaceAll(SCEVExpr::simplify);
        SCEVManager.simplifying.remove(this);

        if (operands.get(0) instanceof SCEVConstant && operands.get(1) instanceof SCEVConstant) {
            return new SCEVConstant(((SCEVConstant) operands.get(0)).value - ((SCEVConstant) operands.get(1)).value);
        } else if (operands.get(1) instanceof SCEVConstant rhs && rhs.value == 0) {
            return operands.getFirst();
        }
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

    @Override
    public Value getValueProjected() {
        return valueProjected;
    }

    @Override
    public void instantiation(BasicBlock parentBB) {
        if (getValueProjected() != null) {
            throw new RuntimeException("已经有值映射，不允许再实例化");
        }
        for (SCEVExpr op : operands) {
            if (op.getValueProjected() == null) {
                op.instantiation(parentBB);
            }
        }
        valueProjected = new SubInstr(parentBB.getParentFunc().getAndAddRegIdx(),
                operands.get(0).getValueProjected(), operands.get(1).getValueProjected(), parentBB);
        valueProjected.setUse(operands.get(0).getValueProjected());
        valueProjected.setUse(operands.get(1).getValueProjected());
        valueProjected.insertBefore(parentBB.getLastInstr());
    }
}