package midend.DCE;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.AtomaticAddInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.terminator.Terminator;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.pseudoinstr.VecStoreInstr;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Queue;
import java.util.Set;

/**
 * 注意：必须考虑VecStoreInstr
 */
public class RemoveDeadCode {
    public static void execute(List<Function> functions) {
        RemoveDeadArray.execute(functions);
        for (Function function : functions) {
            execute4func(function);
        }
    }

    public static void execute4func(Function function) {
        boolean toContinue = true;
        while (toContinue) {
            toContinue = false;

            Set<Instruction> usefulIns = new HashSet<>();

            for (BasicBlock basicBlock : function.getBasicBlockList()) {
                for (Instruction ins : basicBlock.getInstrList()) {
                    if (ins instanceof StoreInstr || ins instanceof CallInstr || ins instanceof Terminator
                            || ins instanceof VecStoreInstr || ins instanceof AtomaticAddInstr) {
                        usefulIns.add(ins);
                    }
                }
            }

            Queue<Instruction> workList = new LinkedList<>(usefulIns);
            while (!workList.isEmpty()) {
                Instruction cur = workList.poll();
                for (Value used : cur.getUsedList()) {
                    if (used instanceof Instruction usedIns && !usefulIns.contains(usedIns)) {
                        workList.add(usedIns);
                        usefulIns.add(usedIns);
                    }
                }
            }

            for (BasicBlock basicBlock : function.getBasicBlockList()) {
                List<Instruction> instructions = new ArrayList<>(basicBlock.getInstrList());
                for (Instruction ins : instructions) {
                    if (!usefulIns.contains(ins)) {
                        ins.forceRemoveFromList();
                        toContinue = true;
                    }
                }
            }
        }
    }
}