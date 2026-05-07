package midend.Analysis.SCEV.Expr;

import frontend.ir.Value;
import frontend.ir.instr.binop.AddInstr;
import frontend.ir.instr.binop.SubInstr;
import midend.Analysis.LoopInfo;

import java.util.List;

public class SCEVVecMinusRecExpr extends SCEVExpr{
    /**
     * 向量化累加表达式
     * 例如：
     * %init = ...
     * loop:
     * %i0 = phi i32 [ %init, %entry ], [ %i4, %loop ]
     * %i1 = add i32 %i0, 1
     * %i2 = add i32 %i1, 1
     * %i3 = add i32 %i2, 1
     * %i4 = add i32 %i3, 1
     * 此时，op0为init，后续4个为add
     */
    private final List<SCEVExpr> operands;
    private final List<SubInstr> subs;
    private final Value valueProjected;
    private final LoopInfo loop;

    public SCEVVecMinusRecExpr(List<SCEVExpr> ops, List<SubInstr> subs, LoopInfo loop, Value valueProjected) {
        this.operands = ops;
        this.valueProjected = valueProjected;
        this.loop = loop;
        this.subs = subs;
    }

    public List<SCEVExpr> getOperands() {
        return operands;
    }

    public List<SubInstr> getSubs() {
        return subs;
    }

    @Override
    public String toString() {
        return "SCEVVecMinusRecExpr " + operands.stream().map(Object::toString).toList() + "<loop" + loop.getCounter() + ">";
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
