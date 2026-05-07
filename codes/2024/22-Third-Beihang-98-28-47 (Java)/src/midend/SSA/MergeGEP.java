package midend.SSA;

import backend.itemStructure.Group;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.AddInstr;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.ArrayList;

/**
 * 用来将 GEP 合并，便于判断两个 GEP 是否指向同一个地址，保证一个地址，一次取完（在可合并的情况下）
 */
public class MergeGEP {
    public static void execute(ArrayList<Function> functions) {
        if (functions == null) {
            throw new NullPointerException();
        }
        for (Function function : functions) {
            BasicBlock basicBlock = (BasicBlock) function.getBasicBlocks().getHead();
            while (basicBlock != null) {
                Instruction instruction = (Instruction) basicBlock.getInstructions().getHead();
                while (instruction != null) {
                    if (instruction instanceof GEPInstr) {
                        if (((GEPInstr) instruction).isUseless()) {
                            instruction.replaceUseTo(((GEPInstr) instruction).getPtrVal());
                            instruction.removeFromList();
                            instruction = (Instruction) instruction.getNext();
                            continue;
                        }
                        
                        Group<AddInstr, GEPInstr> mergedGroup = ((GEPInstr) instruction).tryMergePtr(function);
                        if (mergedGroup != null) {
                            AddInstr link = mergedGroup.getFirst();
                            GEPInstr merged = mergedGroup.getSecond();
                            link.insertBefore(instruction);
                            merged.insertBefore(instruction);
                            instruction.replaceUseTo(merged);
                            instruction.removeFromList();
                        }
                    }
                    instruction = (Instruction) instruction.getNext();
                }
                basicBlock = (BasicBlock) basicBlock.getNext();
            }
        }
    }
}
