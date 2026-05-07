package midend.SSA;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.cmp.Cmp;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Queue;

/**
 * 整理所有块的跳转条件，从而让一个块知道进入自己所必须满足的条件
 */
public class ConditionBroadcast {
    public static void execute(ArrayList<Function> functions) {
        for (Function function : functions) {
            BasicBlock blk = (BasicBlock) function.getBasicBlocks().getHead();
            while (blk != null) {
                HashSet<BasicBlock> pres = blk.getPres();
                for (BasicBlock pre : pres) {
                    Instruction terminator = pre.getEndInstr();
                    if (terminator instanceof BranchInstr) {
                        Value condition = ((BranchInstr) terminator).getCond();
                        if (!(condition instanceof Cmp)) {
                            throw new RuntimeException("我觉得条件判断都是比较");
                        }
                        if (blk.equals(((BranchInstr) terminator).getThenTarget())) {
                            blk.addCond(pre, condition);
                        } else {
                            // 这个新指令实际上不会被添加到指令序列中，只是一个用于判断的形式，因此不需要正式的寄存器编号
                            blk.addCond(pre, ((Cmp) condition).getReverseCmp(-1));
                        }
                        
                    }
                }
                blk = (BasicBlock) blk.getNext();
            }
        }
    }
    
    
}
