package midend.Analysis.SCEV.Expr;

import frontend.ir.Value;
import frontend.ir.structure.BasicBlock;
import midend.range.Range;

public abstract class SCEVExpr {
    public abstract String toString(); // TODO equals(), etc.

    public abstract SCEVExpr simplify();

    public abstract Value getValueProjected();

    public void instantiation(BasicBlock parentBB) {
        if (getValueProjected() != null) {
            throw new RuntimeException("已经有值映射，不允许再实例化");
        } else {
            throw new RuntimeException("无法实例化表达式: " + this);
        }
    }

    public void replaceComputing(SCEVExpr computing, SCEVExpr replacing) {
        throw new RuntimeException("无法替换表达式: " + this);
    }

    public static boolean isAny = false;

    public Range getRange() {
        int init = getInitValue();
        if (isAny) return Range.top();
        int c1 = calc(1);
        if (isAny) return Range.top();
        int c0 = calc(0);
        if (isAny) return Range.top();
        if (c1 - c0 > 0) {
            return new Range(init, Integer.MAX_VALUE);
        } else {
            return new Range(Integer.MIN_VALUE, init);
        }
    }

    private int calc(int n) {
        return dfs(n, 0, 1);
    }

    private int dfs(int n, int k, int coeff) {
        if (k > n) return 0;

        SCEVExpr simplified = this.simplify();

        if (simplified instanceof SCEVConstant) {
            return ((SCEVConstant) simplified).getValue() * coeff;
        } else if (simplified instanceof SCEVAddRecExpr addRecExpr) {
            int sum = 0;
            for (SCEVExpr operand : addRecExpr.operands) {
                sum += operand.dfs(n, k, coeff);
                if (isAny) return 0;
                // 更新组合系数 C(n, k+1) = C(n, k) * (n-k) / (k+1)
                coeff = coeff * (n - k) / (k + 1);
                k++;
            }

            return sum;
        } else {
            isAny = true;
            return 0;
        }
    }

    private int getInitValue() {
        SCEVExpr simplified = this.simplify();
        if (simplified instanceof SCEVConstant scevConstant) {
            return scevConstant.getValue();
        } else if (simplified instanceof SCEVAddRecExpr addRecExpr) {
            return addRecExpr.operands.get(0).getInitValue();
        } else {
            isAny = true;
            return 0;
        }
    }
}
