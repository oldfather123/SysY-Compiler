package midend.Analysis;

import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.AddInstr;
import frontend.ir.instr.otherop.CmpInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.terminator.Terminator;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.Analysis.SCEV.Expr.SCEVAddRecExpr;
import midend.Analysis.SCEV.SCEVManager;
import midend.CFG;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * 请在循环展开前使用，使用前请跑完tripCountAnalyse、LCSSA, SCEV
 */
public class LoopContentAnalyse {
    public static void execute(List<Function> functions) {
        for (Function func : functions) {
            analyseFunc(func);
        }
    }

    public static void analyseFunc(Function func) {
        for (LoopInfo loop : func.getAncientLoopInfo()) {
            analyseLoop(loop);
        }

    }

    public static void analyseLoop(LoopInfo loop) {
        for (LoopInfo childLoop : loop.getChildLoop()) {
            analyseLoop(childLoop);
        }
        loop.loopContents = null;
        if (loop.isSimpleLoop() && loop.isConditionSimple() && loop.isControlFlowSimple()) {
            loop.loopContents = new ArrayList<>();
            Set<Instruction> usedInLCSSA = loop.usedInLCSSA();
            SCEVManager manager = loop.getFunction().getSCEVManager();
            for (BasicBlock bb : loop.getAllBlocksSorted()) {
                for (Instruction instr : bb.getInstrList()) {
                    if (instr instanceof PhiInstr || instr instanceof CmpInstr) {
                        // phi只会在循环头现现
                        if (usedInLCSSA.contains(instr)) {
                            // 说明变量被外面用了
                            loop.loopContents.add(instr);
                        }
                    } else if (instr instanceof Terminator) {
                        continue;
                    } else if (instr instanceof AddInstr add) {
                        if (!((manager.getSCEV(add.getOperand1()) instanceof SCEVAddRecExpr
                                || manager.getSCEV(add.getOperand2()) instanceof SCEVAddRecExpr) && !usedInLCSSA.contains(add))) {
                            loop.loopContents.add(instr);
                        }
                    } else {
                        loop.loopContents.add(instr);
                    }
                }
            }
        }
//        if (loop.loopContents != null) {
//            System.out.println(loop.getCounter() + ": ");
//            for (int i = 0; i < loop.loopContents.size(); i++) {
//                System.out.println(loop.loopContents.get(i));
//            }
//        }
    }
}
