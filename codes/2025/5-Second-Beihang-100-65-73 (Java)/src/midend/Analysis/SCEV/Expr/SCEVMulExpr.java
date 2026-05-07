package midend.Analysis.SCEV.Expr;

import frontend.ir.Value;
import frontend.ir.instr.binop.MulInstr;
import frontend.ir.structure.BasicBlock;

import java.util.List;

public class SCEVMulExpr extends SCEVExpr {
    public Value valueProjected;
    public final List<SCEVExpr> operands;

    public SCEVMulExpr(List<SCEVExpr> ops, Value valueProjected) {
        this.valueProjected = valueProjected;
        this.operands = ops;
    }

    public String toString() {
        return "(" + operands.get(0) + " / " + operands.get(1) + ")";
    }

    @Override
    public SCEVExpr simplify() {
        operands.replaceAll(SCEVExpr::simplify);

        if (operands.get(0) instanceof SCEVConstant && operands.get(1) instanceof SCEVConstant) {
            return new SCEVConstant(((SCEVConstant) operands.get(0)).value * ((SCEVConstant) operands.get(1)).value);
        } else if (operands.get(1) instanceof SCEVConstant rhs && rhs.value == 1) {
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
        valueProjected = new MulInstr(parentBB.getParentFunc().getAndAddRegIdx(),
                operands.get(0).getValueProjected(), operands.get(1).getValueProjected(), parentBB);
        valueProjected.setUse(operands.get(0).getValueProjected());
        valueProjected.setUse(operands.get(1).getValueProjected());
        valueProjected.insertBefore(parentBB.getLastInstr());
    }
}