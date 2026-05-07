package midend.DCE;

import frontend.ir.Value;
import frontend.ir.constant.BoolConst;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.CmpInstr;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.range.RangeAnalysisV3;
import midend.range.RangeAnalysisWorklistV3;
import midend.range.Range;

import java.util.ArrayList;
import java.util.List;

/**
 * 前置：RangeAnalysis
 * 后置：CFG
 */

public class RemoveDeadBranch {

    public static void execute(List<Function> functions) {
        for (Function f : functions) {
            if (f instanceof Function.LibFunc) continue;
            simplifyOnFunc(f);
        }
    }

    private static void simplifyOnFunc(Function f) {
        // 1) 一次性取出RA，后面直接用 ctxAfter + ctx.get
        RangeAnalysisWorklistV3 ra = RangeAnalysisV3.getWorklist(f);
        if (ra == null) {
            // 如果管理器里还没有，跑一遍也只跑一次
            ra = RangeAnalysisWorklistV3.runOn(f);
        }

        final List<BasicBlock> blocks = new ArrayList<>(f.getBasicBlockList());
        final List<Rewrite> rewrites = new ArrayList<>();

        // 2) 单次扫描：利用ctx判断是否可去分支
        for (BasicBlock bb : blocks) {

            Instruction term = bb.getLastInstr();
            if (!(term instanceof BranchInstr br)) continue;

            Value cond = br.getCondition();

            if (!(cond instanceof CmpInstr icmp) || !icmp.isI32()) continue;


            // 常量/同值快速路径
            Tri quick = quickProve(icmp);
            if (quick == Tri.UNKNOWN) {
                // 用 RA：对ctx读取操作数区间
                Range l = RangeAnalysisV3.getRange(icmp.getOperand1(), bb);
                Range r = RangeAnalysisV3.getRange(icmp.getOperand2(), bb);

                quick = prove(icmp.getCond(), l, r);
            }

            if (quick == Tri.TRUE) {
                rewrites.add(new Rewrite(bb, br.getThenBlk()));
            } else if (quick == Tri.FALSE) {
                rewrites.add(new Rewrite(bb, br.getElseBlk()));
            }
        }

        if (rewrites.isEmpty()) return;

        // 3) 批量改写 + 统一做一次CFG清理
        for (Rewrite rw : rewrites) rewriteToUncond(rw.bb, rw.target);

        // 做至多一次“再迭代”
        // 由于CFG变化，RA上下文也可能改；再跑一轮能捕到更多简化，但限制迭代次数避免开销
        RangeAnalysisWorklistV3 ra2 = RangeAnalysisWorklistV3.runOn(f);
        final List<Rewrite> rewrites2 = new ArrayList<>();
        for (BasicBlock bb : new ArrayList<>(f.getBasicBlockList())) {
            Instruction term = bb.getLastInstr();
            if (!(term instanceof BranchInstr br)) continue;
            Value cond = br.getCondition();
            if (!(cond instanceof CmpInstr icmp) || !icmp.isI32()) continue;

            Tri quick = quickProve(icmp);
            if (quick == Tri.UNKNOWN) {
                Range l = RangeAnalysisV3.getRange(icmp.getOperand1(), bb);
                Range r = RangeAnalysisV3.getRange(icmp.getOperand2(), bb);
                quick = prove(icmp.getCond(), l, r);
            }
            if (quick == Tri.TRUE) rewrites2.add(new Rewrite(bb, br.getThenBlk()));
            else if (quick == Tri.FALSE) rewrites2.add(new Rewrite(bb, br.getElseBlk()));
        }
        if (!rewrites2.isEmpty()) {
            for (Rewrite rw : rewrites2) rewriteToUncond(rw.bb, rw.target);
        }
    }

    private record Rewrite(BasicBlock bb, BasicBlock target) {
    }

    /*private static Range getRange(RangeAnalysisWorklistV2.Context ctx, Value v) {
        // 常量快速返回
        if (v instanceof IntConst c) return new Range(c.getConstInt().intValue(), c.getConstInt().intValue());
        // 其他从上下文拿（若无条目，视为TOP）
        RangeAnalysisWorklistV2.Range rr = ctx.getOrDefaultTop(v);
        if (rr == null || rr.isTop()) return Range.top();
        return new Range(rr.min(), rr.max());
    }*/

    enum Tri {TRUE, FALSE, UNKNOWN}

    /**
     * 更快的常量/同值短路：不落到区间推理
     */
    private static Tri quickProve(CmpInstr icmp) {
        Value a = icmp.getOperand1(), b = icmp.getOperand2();

        // 同一值比较
        if (a == b) {
            return switch (icmp.getCond()) {
                case IEQ, ILE, IGE -> Tri.TRUE;
                case INE, ILT, IGT -> Tri.FALSE;
                default -> Tri.UNKNOWN;
            };
        }

        // 两侧常量
        if (a instanceof IntConst la && b instanceof IntConst rb) {
            int x = la.getConstInt().intValue(), y = rb.getConstInt().intValue();
            return switch (icmp.getCond()) {
                case IEQ -> (x == y) ? Tri.TRUE : Tri.FALSE;
                case INE -> (x != y) ? Tri.TRUE : Tri.FALSE;
                case ILT -> (x < y) ? Tri.TRUE : Tri.FALSE;
                case ILE -> (x <= y) ? Tri.TRUE : Tri.FALSE;
                case IGT -> (x > y) ? Tri.TRUE : Tri.FALSE;
                case IGE -> (x >= y) ? Tri.TRUE : Tri.FALSE;
                default -> Tri.UNKNOWN;
            };
        }
        return Tri.UNKNOWN;
    }

    /**
     * 区间推理（保守正确）
     */
    private static Tri prove(CmpInstr.CmpCond cond, Range L, Range R) {
        if (L.isTop() || R.isTop()) return Tri.UNKNOWN;

        int lmin = L.min, lmax = L.max, rmin = R.min, rmax = R.max;
        return switch (cond) {
            case IEQ -> {
                if (lmax < rmin || rmax < lmin) yield Tri.FALSE;
                if (lmin == lmax && rmin == rmax && lmin == rmin) yield Tri.TRUE;
                yield Tri.UNKNOWN;
            }
            case INE -> {
                if (lmax < rmin || rmax < lmin) yield Tri.TRUE;
                if (lmin == lmax && rmin == rmax && lmin == rmin) yield Tri.FALSE;
                yield Tri.UNKNOWN;
            }
            case ILT -> {
                if (lmax < rmin) yield Tri.TRUE;
                if (lmin >= rmax) yield Tri.FALSE;
                yield Tri.UNKNOWN;
            }
            case ILE -> {
                if (lmax <= rmin) yield Tri.TRUE;
                if (lmin > rmax) yield Tri.FALSE;
                yield Tri.UNKNOWN;
            }
            case IGT -> {
                if (lmin > rmax) yield Tri.TRUE;
                if (lmax <= rmin) yield Tri.FALSE;
                yield Tri.UNKNOWN;
            }
            case IGE -> {
                if (lmin >= rmax) yield Tri.TRUE;
                if (lmax < rmin) yield Tri.FALSE;
                yield Tri.UNKNOWN;
            }
            default -> Tri.UNKNOWN;
        };
    }

    /**
     * 把条件分支改成无条件跳转（不反复触发CFG清理）
     */
    private static void rewriteToUncond(BasicBlock bb, BasicBlock target) {
//        BranchInstr old = (BranchInstr) bb.getLastInstr();
//        old.forceRemoveFromList();
//        new JumpInstr(target, bb);
        BranchInstr old = (BranchInstr) bb.getLastInstr();
        Value oldCond = old.getCondition();
        Value newCond;
        if (old.getThenBlk() == target) {
            newCond = new BoolConst(1);
        } else {
            newCond = new BoolConst(0);
        }
        old.modifyUse(oldCond, newCond);
        oldCond.getUserList().remove(old);
        newCond.getUserList().add(old);
        old.simplify();
    }
}
