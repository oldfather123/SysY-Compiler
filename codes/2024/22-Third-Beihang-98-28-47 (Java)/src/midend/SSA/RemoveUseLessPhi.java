package midend.SSA;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.ArrayList;

public class RemoveUseLessPhi {
    public static boolean execute(ArrayList<Function> functions) {
        boolean toBeContinue = false;
        for (Function function : functions) {
            toBeContinue = removeUnused(function) | toBeContinue;
        }
        return toBeContinue;
    }

    private static boolean removeUnused(Function function) {
        //phi的block的setUse应该是要加的
        boolean toBeContinue = false;
        BasicBlock blk = (BasicBlock) function.getBasicBlocks().getHead();
        while (blk != null) {
            Instruction instr = (Instruction) blk.getInstructions().getHead();
            while (instr instanceof PhiInstr) {
                ArrayList<BasicBlock> toRemove = new ArrayList<>();
                for (BasicBlock remove : ((PhiInstr) instr).getPrtBlks()) {
                    if (!blk.getPres().contains(remove)) {
                        toRemove.add(remove);
                        toBeContinue = true;
                    }
                }
                for (BasicBlock remove : toRemove) {
                    instr.removeUse(remove);
                }
                if (((PhiInstr) instr).getPrtBlks().isEmpty()) {
                    instr.removeFromList();
                    toBeContinue = true;
                } else if (((PhiInstr) instr).canSimplify()) {
                    Value value = ((PhiInstr) instr).getValues().get(0);
                    instr.replaceUseTo(value);
                    instr.removeFromList();
                    toBeContinue = true;
                }
                instr = (Instruction) instr.getNext();
            }
            blk = (BasicBlock) blk.getNext();
        }
        return toBeContinue;
    }

    public static void clear4branchModify(BasicBlock blk) {
        Instruction instr = (Instruction) blk.getInstructions().getHead();
        while (instr instanceof PhiInstr) {
            ArrayList<BasicBlock> toRemove = new ArrayList<>();
            for (BasicBlock remove : ((PhiInstr) instr).getPrtBlks()) {
                if (!blk.getPres().contains(remove)) {
                    toRemove.add(remove);
                }
            }
            for (BasicBlock remove : toRemove) {
                instr.removeUse(remove);
            }
            if (((PhiInstr) instr).getPrtBlks().isEmpty()) {
                instr.removeFromList();
            } else if (((PhiInstr) instr).canSimplify()) {
                Value value = ((PhiInstr) instr).getValues().get(0);
                instr.replaceUseTo(value);
                instr.removeFromList();
            }
            instr = (Instruction) instr.getNext();
        }
    }

}
