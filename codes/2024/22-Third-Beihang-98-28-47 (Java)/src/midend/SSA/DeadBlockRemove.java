package midend.SSA;

import frontend.ir.instr.Instruction;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.ArrayList;

public class DeadBlockRemove {
    public static void execute(ArrayList<Function> functions) {
        for (Function function : functions) {
            removeBlk(function);
        }
    }

    private static void removeBlk(Function function) {
        BasicBlock secondBlk = (BasicBlock) function.getBasicBlocks().getHead().getNext();
        //删除不要的块
        while (secondBlk != null) {
            dfs4remove(secondBlk);
            secondBlk = (BasicBlock) secondBlk.getNext();
        }

    }

    private static void dfs4remove(BasicBlock block) {
        if (block.getBeginUse() == null) {
            Instruction instr = block.getEndInstr();
            block.removeFromList();
            //把我用到的所有block的use信息删掉
            if (instr instanceof JumpInstr) {
                dfs4remove(((JumpInstr) instr).getTarget());
            } else if (instr instanceof BranchInstr) {
                dfs4remove(((BranchInstr) instr).getThenTarget());
                dfs4remove(((BranchInstr) instr).getElseTarget());
            }
        }
    }
}
