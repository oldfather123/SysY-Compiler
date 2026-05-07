package midend.SSA;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.BinaryOperation;
import frontend.ir.instr.binop.SRemInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.structure.Procedure;

import java.util.ArrayList;

/**
 * Operation instruction simplification
 * 运算指令简化
 */
public class OIS {
    public static void execute(ArrayList<Function> functions) {
        if (functions == null) {
            throw new NullPointerException();
        }
        for (Function function : functions) {
            BasicBlock begin = (BasicBlock) function.getBasicBlocks().getHead();
            BasicBlock end   = (BasicBlock) function.getBasicBlocks().getTail();
            OSI4blks(begin, end);
        }
    }
    
    public static void OSI4blks(BasicBlock begin, BasicBlock end) {
        BasicBlock stop = (BasicBlock) end.getNext();
        BasicBlock basicBlock = begin;
        while (basicBlock != stop) {
            Instruction instruction = (Instruction) basicBlock.getInstructions().getHead();
            while (instruction != null) {
                simpleOIS4ins(instruction);
                
                // 针对累加取模操作的特殊化简
                if (instruction instanceof SRemInstr) {
                    Procedure curPro = (Procedure) basicBlock.getParent().getOwner();
                    BinaryOperation newIns = ((SRemInstr) instruction).sumSimplify(curPro);
                    if (newIns != null) {
                        newIns.insertBefore(instruction);
                    }
                }
                instruction = (Instruction) instruction.getNext();
            }
            basicBlock = (BasicBlock) basicBlock.getNext();
        }
    }
    
    public static void simpleOIS4ins(Instruction instruction) {
        Value simplified = instruction.operationSimplify();
        if (simplified != null) {
            instruction.replaceUseTo(simplified);
            instruction.removeFromList();
        }
        
    }
}
