package midend.Analysis.SCEV;

import frontend.ir.instr.Instruction;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.Analysis.SCEV.Expr.SCEVExpr;
import midend.CFG;

import java.util.List;

public class SCEVAnalyse {

    public static void execute(List<Function> functions, boolean toPrintInfo) {
        for (Function func : functions) {
            analyseFunc(func, toPrintInfo);
        }
    }

    public static void analyseFunc(Function func, boolean toPrintInfo) {
        SCEVManager scevManager = new SCEVManager();
        func.setSCEVManager(scevManager);
        List<BasicBlock> rpo = CFG.getReversePostOrder(func);
        for (BasicBlock bb : rpo) {
            for (Instruction instr : bb.getInstrList()) {
                if (instr.getRegIndex() != null) {
                    SCEVExpr expr = scevManager.getSCEV(instr);
                    if (instr.getRegIndex() < 1000 && toPrintInfo) {
                        System.out.println(instr + " " + expr);
                    }
                }
            }
        }
    }
}
