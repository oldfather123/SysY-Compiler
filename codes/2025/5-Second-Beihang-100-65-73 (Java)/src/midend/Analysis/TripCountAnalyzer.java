package midend.Analysis;

import frontend.ir.Value;
import frontend.ir.instr.otherop.CmpInstr;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.instr.terminator.Terminator;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.Analysis.SCEV.LoopCondition;
import midend.Analysis.SCEV.SCEVManager;

import java.util.List;

public class TripCountAnalyzer {
    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            analyzeFunc(function);
        }
    }

    public static void analyzeFunc(Function function) {
        for (LoopInfo loop : function.getAncientLoopInfo()) {
            analyzeLoopPrecise(loop);
//            checkLoop(loop);
        }
    }

    /**
     * 只能处理唯一出口情况
     */
    public static void analyzeLoopPrecise(LoopInfo loopInfo) {
        for (LoopInfo childLoop : loopInfo.getChildLoop()) {
            analyzeLoopPrecise(childLoop);
        }
        BranchInstr branch = null;
        for (BasicBlock exitingBlock : loopInfo.getExiting()) {
            Terminator terminator = (Terminator) exitingBlock.getLastInstr();
            if (terminator instanceof BranchInstr br && branch == null) {
                branch = br;
            } else {
                loopInfo.setLoopCondition(null);
                return;
            }
        }

        SCEVManager manager = loopInfo.getFunction().getSCEVManager();
        Value cond;
        if (branch != null) {
            cond = branch.getCondition();
        } else {
            throw new RuntimeException("branch must have cond");
        }
        if (cond instanceof CmpInstr cmp) {
            LoopCondition loopCondition = new LoopCondition(branch, cmp);
            loopInfo.setLoopCondition(loopCondition);
            manager.getTripCount(loopCondition, loopInfo);
            if (loopCondition.status) {
                loopCondition.tripCountExpr = loopCondition.tripCountExpr.simplify();
            }
        } else {
            // todo:无法处理条件非cmp的情况
            loopInfo.setLoopCondition(null);
        }

    }



    public static void checkLoop(LoopInfo loopInfo) {
        for (LoopInfo childLoop : loopInfo.getChildLoop()) {
            checkLoop(childLoop);
        }
        if (loopInfo.loopCondition != null) {
//            System.out.println(loopInfo.tailLoop);
        }
    }

}
