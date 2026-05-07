package midend.optimism;

import midend.BasicBlock;
import midend.Function;
import midend.Module;
import midend.analysis.FunctionSideEffect;
import midend.instr.CallInstr;
import midend.instr.Instruction;

import java.util.ArrayList;

public class DeadCodeRemove {
    private Module module;
    private ArrayList<Instruction> useful = new ArrayList<>();

    public DeadCodeRemove(Module module) {
        this.module = module;
    }

    public void run() {
        FunctionSideEffect.sideEffectAnalysis(module);
        for (Function function : module.getFunctions()) {
            boolean change = true;
            while (change) {
                useful.clear();
                change = false;
                for (BasicBlock block : function.getBlockList()) {
                    ArrayList<Instruction> toBeRemoved = new ArrayList<>();
                    for (Instruction instruction : block.getInstrList()) {
                        if (instruction.canBeUsed() || (instruction instanceof CallInstr && !((CallInstr) instruction).getFunction().isSideEffect())) {
                            //TODO:call指令也可以删除，之后再写
//                            if (instruction instanceof CallInstr) {
//                                System.out.println("");
//                            }
                            if (instruction.getUseList().isEmpty()) {
                                toBeRemoved.add(instruction);
                            }
                        }
                    }
                    for (Instruction instruction : toBeRemoved) {
                        instruction.remove();
                        change = true;
                    }
                }
            }
        }
    }
}