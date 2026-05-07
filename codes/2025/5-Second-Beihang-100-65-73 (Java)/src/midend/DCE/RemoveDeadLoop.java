package midend.DCE;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.memop.AllocaInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.terminator.ReturnInstr;
import frontend.ir.instr.terminator.Terminator;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.Analysis.LoopInfo;
import midend.FunctionInfoCollector;

import java.util.HashSet;
import java.util.List;
import java.util.Set;

// todo: 后置：cfg

public class RemoveDeadLoop {
    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            if (function instanceof Function.LibFunc) continue;
            Set<LoopInfo> needRemoveLoops = new HashSet<>();
            for (LoopInfo topLoopInfo : function.getAncientLoopInfo()) {
                if (needRemove(topLoopInfo)) {
                    needRemoveLoops.add(topLoopInfo);
                }
            }
            needRemoveLoops.forEach(function.getAncientLoopInfo()::remove);
        }
    }

    public static boolean needRemove(LoopInfo loopInfo) {
        Set<LoopInfo> needRemoveLoops = new HashSet<>();
        for (LoopInfo sub : loopInfo.getChildLoop()) {
            if (needRemove(sub)) {
                needRemoveLoops.add(sub);
            }
        }
        needRemoveLoops.forEach(loopInfo.getChildLoop()::remove);

        if (!isLoopDead(loopInfo)) return false;

        BasicBlock preHeader = loopInfo.getPreHeader();
        BasicBlock header = loopInfo.getLoopHeader();
        BasicBlock exitTarget = loopInfo.getExitTargets().iterator().next();

        if (preHeader == null) {
            for (BasicBlock enter : loopInfo.getEnterFroms()) {
                enter.replaceJumpTarget(header, exitTarget);
            }
        } else {
            ((Terminator) (preHeader.getLastInstr())).replaceJumpTarget(header, exitTarget);
        }
        for (BasicBlock toDelBlock : loopInfo.getBodyBlocks()) {
            toDelBlock.removeFromListClear();
        }
        return true;
    }

    public static boolean isLoopDead(LoopInfo loopInfo) {
        if (!loopInfo.getChildLoop().isEmpty()) return false;
        if (loopInfo.getExitTargets().size() != 1) return false;
        BasicBlock onlyExitTarget = loopInfo.getExitTarget();
        if (onlyExitTarget.getPres().size() != 1) return false;
        if (!onlyExitTarget.getInstrList().isEmpty() && onlyExitTarget.getFirstInstr() instanceof PhiInstr)
            return false;

        for (BasicBlock block : loopInfo.getBodyBlocks()) {
            for (Instruction instruction : block.getInstrList()) {
                if (instruction instanceof StoreInstr) return false;
                if (instruction instanceof ReturnInstr) return false;
                if (instruction instanceof CallInstr callInstr &&
                        !FunctionInfoCollector.getFuncInfo(callInstr.getCallee()).isPureInRemoveDeadLoop(callInstr))
                    return false;
                if(instruction instanceof LoadInstr loadInstr && !(loadInstr.getPointer() instanceof AllocaInstr)) return false;
                for (Value user : instruction.getUserList()) {
                    if (user instanceof Instruction userInstr) {
                        if (userInstr.getParentBB().getLoopBelonged() != loopInfo) return false;
                    }
                }
            }
        }

        return true;
    }
}
