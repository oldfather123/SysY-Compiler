package midend.Analysis.SCEV.Expr;

import frontend.ir.Value;
import frontend.ir.instr.binop.AddInstr;
import frontend.ir.structure.BasicBlock;
import midend.Analysis.SCEV.SCEVManager;

import java.util.List;

public class SCEVAddExpr extends SCEVExpr {
    public final List<SCEVExpr> operands;
    public Value valueProjected;

    public SCEVAddExpr(List<SCEVExpr> ops, Value valueProjected) {
        this.operands = ops;
        this.valueProjected = valueProjected;
    }

    public String toString() {
        return "(" + String.join(" + ", operands.stream().map(Object::toString).toList()) + ")";
    }

    @Override
    public SCEVExpr simplify() {
        if (SCEVManager.simplifying.contains(this)) {
            return this;
        }
        SCEVManager.simplifying.add(this);
        operands.replaceAll(SCEVExpr::simplify);
        SCEVManager.simplifying.remove(this);
        if (operands.stream().allMatch(op -> op instanceof SCEVConstant)) {
            return new SCEVConstant(operands.stream().mapToInt(op -> ((SCEVConstant) op).value).sum());
        }
        if (operands.getFirst() instanceof SCEVConstant con && con.value == 0) {
            return operands.getLast();
        } else if (operands.getLast() instanceof SCEVConstant con && con.value == 0) {
            return operands.getFirst();
        }
        // 只有约分完还有映射才有意义。。。吗？
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
        valueProjected = new AddInstr(parentBB.getParentFunc().getAndAddRegIdx(),
                operands.get(0).getValueProjected(), operands.get(1).getValueProjected(), parentBB);
        valueProjected.setUse(operands.get(0).getValueProjected());
        valueProjected.setUse(operands.get(1).getValueProjected());
        valueProjected.insertBefore(parentBB.getLastInstr());
    }
}
