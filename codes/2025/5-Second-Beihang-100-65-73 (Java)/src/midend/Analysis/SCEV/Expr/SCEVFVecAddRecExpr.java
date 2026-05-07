package midend.Analysis.SCEV.Expr;

import frontend.ir.Value;
import frontend.ir.instr.binop.FAddInstr;
import midend.Analysis.LoopInfo;

import java.util.List;

public class SCEVFVecAddRecExpr extends SCEVExpr{
    /**
     * 向量化累加表达式
     * 例如：
     * %init = ...
     * loop:
     * %i0 = phi float [ %init, %entry ], [ %i4, %loop ]
     * %i1 = add float %i0, 1.0
     * %i2 = add float %i1, 1.0
     * %i3 = add float %i2, 1.0
     * %i4 = add float %i3, 1.0
     * 此时，op0为init，后续4个为add
     */
    private final List<SCEVExpr> operands;
    private final List<FAddInstr> adds;
    private final Value valueProjected;
    private final LoopInfo loop;

    public SCEVFVecAddRecExpr(List<SCEVExpr> ops, List<FAddInstr> adds, LoopInfo loop, Value valueProjected) {
        this.operands = ops;
        this.valueProjected = valueProjected;
        this.loop = loop;
        this.adds = adds;
    }

    public List<SCEVExpr> getOperands() {
        return operands;
    }

    public List<FAddInstr> getAdds() {
        return adds;
    }

    @Override
    public String toString() {
        return "SCEVFVecAddRecExpr " + operands.stream().map(Object::toString).toList() + "<loop" + loop.getCounter() + ">";
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
