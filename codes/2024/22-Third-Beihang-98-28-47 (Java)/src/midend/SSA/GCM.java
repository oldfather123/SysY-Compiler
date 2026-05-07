package midend.SSA;

import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.terminator.Terminator;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.HashSet;

/**
 * GCM(Global Code Motion)
 * 全局代码移动（全局代码提升）
 */
public class GCM {
    private static final HashSet<Instruction> visited = new HashSet<>();
    
    public static void doGCM(HashSet<Function> functions) {
        if (functions == null) {
            throw new NullPointerException();
        }
        for (Function function : functions) {
            BasicBlock basicBlock = (BasicBlock) function.getBasicBlocks().getHead();
            while (basicBlock != null) {
                Instruction instruction = (Instruction) basicBlock.getInstructions().getHead();
                while (instruction != null) {
                    if (isPinned(instruction)) {
                        visited.add(instruction);
                        
                    }
                    instruction = (Instruction) instruction.getNext();
                }
                basicBlock = (BasicBlock) basicBlock.getNext();
            }
        }
    }
    
    
    
    private static boolean isPinned(Instruction instruction) {
        return instruction instanceof PhiInstr || instruction instanceof Terminator;
    }
}
