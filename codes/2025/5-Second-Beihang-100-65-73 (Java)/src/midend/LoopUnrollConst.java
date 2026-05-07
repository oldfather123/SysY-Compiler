package midend;

import frontend.ir.Value;
import frontend.ir.cloner.UnrollLoopEntire;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.Analysis.LoopInfo;
import midend.Analysis.SCEV.Expr.SCEVConstant;
import midend.DCE.RemoveDeadCode;

import java.util.*;

public class LoopUnrollConst {
    public static final int UNROLL_ALL_MAX = 512;
    public static final int UNROLL_ALL_MAX_LINES = 4000;
    public static final int UNROLL_MAX = 35;
    public static int unrollCount = 0;

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

    public static void releaseLoop(LoopInfo loop) {
        BasicBlock latch = loop.getLatchBlocks().iterator().next();
        BasicBlock exit = loop.getExitTarget();
        latch.getLastInstr().forceRemoveFromList();
        new JumpInstr(exit, latch);
        BasicBlock header = loop.getLoopHeader();
        HashSet<BasicBlock> sucs = new HashSet<>(header.getSucs());
        sucs.remove(exit);
        header.getLastInstr().forceRemoveFromList();
        new JumpInstr(sucs.iterator().next(), header);

        for (Instruction instr : exit.getInstrList()) {
            if (instr instanceof PhiInstr phi) {
                for (BasicBlock fromBB : new HashSet<>(phi.getOperandMap().keySet())) {
                    if (fromBB == header) {
                        Value fromHeader = phi.getOperandMap().get(fromBB);
                        if (fromHeader instanceof PhiInstr fromPhi) {
                            Value fromLatch = fromPhi.getOperandMap().get(latch);
                            phi.modifyUse(fromHeader, fromLatch);
                            fromHeader.getUserList().remove(phi);
                            fromLatch.getUserList().add(phi);
                        }

                        phi.modifyUse(header, latch);
                        header.getUserList().remove(phi);
                        latch.getUserList().add(phi);

                    }
                }
            } else {
                break;
            }
        }

        for (Instruction instr : header.getInstrList()) {
            if (instr instanceof PhiInstr phi) {
                for (BasicBlock fromBB : new HashSet<>(phi.getOperandMap().keySet())) {
                    if (fromBB == latch) {
                        phi.delOpPair(latch);
                    }
                }
            } else {
                break;
            }
        }
        loop.deleteCurrentLoopLite();
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

        // 以下均为单出口
        if (loop.loopCondition.tripCountExpr instanceof SCEVConstant) {
            int tripCount = ((SCEVConstant) loop.loopCondition.tripCountExpr).value;
            if (tripCount == 0) {
                //TODO:有彻底删除循环的必要吗？
//                loop.deleteCurrentLoop();
            } else if (tripCount == 1) {
                releaseLoop(loop);
            } else if (tripCount <= UNROLL_ALL_MAX && tripCount * loop.getInstrNum() <= UNROLL_ALL_MAX_LINES) {
                UnrollLoopEntire.getInstance().unrollLoopEntire(loop, tripCount - 1);
                loop.deleteCurrentLoopLite();
                RemoveDeadCode.execute(List.of(loop.getFunction()));
                unrollCount++;
            }
        }
    }
}
