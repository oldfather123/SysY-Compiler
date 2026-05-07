package midend.SSA;

import frontend.ir.instr.Instruction;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

public class GVN {
    public static void execute(ArrayList<Function> functions) {
        if (functions == null) {
            throw new NullPointerException();
        }
        for (Function function : functions) {
            BasicBlock basicBlock = (BasicBlock) function.getBasicBlocks().getHead();
            dfsIdoms(basicBlock, new HashMap<>());
        }
    }
    
    private static void dfsIdoms(BasicBlock basicBlock, HashMap<String, Instruction> instructions) {
        if (basicBlock == null) {
            throw new NullPointerException();
        }
        
        Instruction instruction = (Instruction) basicBlock.getInstructions().getHead();
        while (instruction != null) {
            if (instructions.containsKey(instruction.myHash())) {
                Instruction basicInstr = instructions.get(instruction.myHash());
                instruction.replaceUseTo(basicInstr);
                instruction.removeFromList();
            } else {
                instructions.put(instruction.myHash(), instruction);
            }
            instruction = (Instruction) instruction.getNext();
        }
        
        HashSet<BasicBlock> idoms = basicBlock.getIDoms();
        for (BasicBlock next : idoms) {
            HashMap<String, Instruction> nextInstrMap = new HashMap<>(instructions);
            dfsIdoms(next, nextInstrMap);
        }
    }
}
