package midend;

import frontend.ir.cloner.UnrollLoopNonEntire;
import frontend.ir.instr.binop.SubInstr;
import frontend.ir.structure.Function;
import midend.Analysis.LoopInfo;
import midend.Analysis.SCEV.Expr.SCEVConstant;
import midend.Analysis.SCEV.Expr.SCEVExpr;
import midend.Analysis.SCEV.Expr.SCEVMinusExpr;
import midend.DCE.RemoveDeadCode;

import java.util.HashSet;
import java.util.List;

public class LoopUnrollN {
    public static final int UNROLL_ALL_MAX_LINES = 500;
    public static final int UNROLL_MAX = 30;
    public static int unrollCount = 0;
    public static final int UNROLL_NON_CONST = 4;

    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            unrollFunction(function);
        }
    }

    public static void unrollFunction(Function function) {
        for (LoopInfo loop : new HashSet<>(function.getAncientLoopInfo())) {
            unrollLoop(loop);
        }
    }

    public static void unrollLoop(LoopInfo loop) {
        for (LoopInfo child : new HashSet<>(loop.getChildLoop())) {
            unrollLoop(child);
        }
        if (unrollCount >= UNROLL_MAX) {
            return;
        }

        if (!(loop.isSimpleLoop() && loop.isConditionSimple())) {
            return;
        }

        SCEVExpr tripCountExpr = loop.loopCondition.tripCountExpr;
        if (tripCountExpr.getValueProjected() == null) {
            if (tripCountExpr instanceof SCEVMinusExpr min) {
                for (SCEVExpr op : min.operands) {
                    if (op.getValueProjected() == null) {
                        return;
                    }
                }
                SubInstr sub = new SubInstr(loop.getFunction().getAndAddRegIdx(),
                        min.operands.get(0).getValueProjected(),
                        min.operands.get(1).getValueProjected(), loop.getPreHeader());
                sub.setUse(min.operands.get(0).getValueProjected());
                sub.setUse(min.operands.get(1).getValueProjected());
                sub.insertBefore(loop.getPreHeader().getLastInstr());
                ((SCEVMinusExpr) tripCountExpr).valueProjected = sub;
            }
        }

        // 以下均为单出口
        if (loop.loopCondition.tripCountExpr.getValueProjected() != null &&
                UNROLL_NON_CONST * loop.getInstrNum() <= UNROLL_ALL_MAX_LINES) {
            if (!(loop.loopCondition.tripCountExpr instanceof SCEVConstant)) {
                UnrollLoopNonEntire.getInstance().unrollLoopNonEntire(loop, UNROLL_NON_CONST);
                RemoveDeadCode.execute(List.of(loop.getFunction()));
                unrollCount++;
            }
        }
    }
}
