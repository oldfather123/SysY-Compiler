package midend.loop;

import frontend.ir.Use;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.BinaryOperation;
import frontend.ir.instr.memop.MemoryOperation;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.ArrayList;

public class RemoveUseLessLoop {
    public static void execute(ArrayList<Function> functions) {
        AnalysisLoop.execute(functions);
        for (Function function : functions) {
            for (Loop out : function.getOuterLoop()) {
                dfs4removeLoop(out);
            }
        }
    }

    private static void dfs4removeLoop(Loop loop) {
        for (Loop inner : loop.getInnerLoops()) {
            dfs4removeLoop(inner);
        }
//        loop.LoopPrint();
        if (canRemove(loop)) {
            BasicBlock preHeader = loop.getPreHeader();
            BasicBlock loopExit = loop.getExits().get(0);
            for (BasicBlock blk : loop.getBlks()) {
                blk.removeFromList();
            }
            preHeader.getEndInstr().removeFromList();
            preHeader.setRet(false);
            preHeader.addInstruction(new JumpInstr(loopExit));
        }

    }

    private static boolean canRemove(Loop loop) {
        if (!loop.isSimpleLoop()) {
            return false;
        }
        for (BasicBlock blk : loop.getBlks()) {
            Instruction instr = (Instruction) blk.getInstructions().getHead();
            while (instr != null) {
                if (!checkNoSideEffect(instr)) {
                    return false;
                }
                Use use = instr.getBeginUse();
                while (use != null) {
                    if (!(loop.getBlks().contains(use.getUser().getParentBB()))) {
                        return false;
                    }
                    use = (Use) use.getNext();
                }
                instr = (Instruction) instr.getNext();
            }
        }
        return true;
    }

    private static boolean checkNoSideEffect(Instruction instr) {
        if (instr instanceof MemoryOperation) {
            return false;
        }
        if (instr instanceof CallInstr) {
            return false;
        }
        return true;
    }
}
