package midend.SSA;

import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.MulInstr;
import frontend.ir.instr.binop.SDivInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;

/**
 * Operation Instruction Strength Reduction
 * 运算指令强度消减
 */
public class OISR {
    public static void doOISR(HashSet<Function> functions) {
        if (functions == null) {
            throw new NullPointerException();
        }
        for (Function function : functions) {
            BasicBlock basicBlock = (BasicBlock) function.getBasicBlocks().getHead();
            while (basicBlock != null) {
                Instruction instruction = (Instruction) basicBlock.getInstructions().getHead();
                while (instruction != null) {
                    ArrayList<Instruction> insList = null;
                    
                    if (instruction instanceof MulInstr) {
                        insList = ((MulInstr) instruction).strengthReduction(function);
                    } else if (instruction instanceof SDivInstr) {
                        insList = ((SDivInstr) instruction).strengthReduction(function);
                    }
                    
                    if (insList != null && !insList.isEmpty()) {
                        Collections.reverse(insList);
                        for (Instruction newIns : insList) {
                            newIns.insertAfter(instruction);
                        }
                        instruction.replaceUseTo(insList.get(0));
                        instruction.removeFromList();
                    }

                    instruction = (Instruction) instruction.getNext();
                }
                basicBlock = (BasicBlock) basicBlock.getNext();
            }
        }
    }
}
