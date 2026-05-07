package midend.optimism;

import frontend.AST.Func;
import midend.*;
import midend.LLVMType.PointerType;
import midend.LLVMType.Type;
import midend.Module;
import midend.instr.AllocaInstr;
import midend.instr.Instruction;

import java.util.ArrayList;

public class ArrayLift {
    private Module module;
    private int num = 0;

    public ArrayLift(Module module) {
        this.module = module;
    }

    public void run() {
        ArrayList<Instruction> delete = new ArrayList<>();
        for (Function function : module.getFunctions()) {
            if (module.getFunctions().size() == 1 && function.getName().equals("@main")) {
                for (BasicBlock block : function.getBlockList()) {
                    for (Instruction instruction : block.getInstrList()) {
                        if (instruction instanceof AllocaInstr && instruction.getBasicBlock().getLoopDepth() == 0) {
                            Type type = ((PointerType) instruction.getType()).getElementType();
                            ArrayList<Value> init = new ArrayList<>();
                            GlobalVar globalVar = new GlobalVar(new PointerType(type), "lift" + String.valueOf(num++), init);
                            module.addGlobalVar(globalVar);
                            instruction.replaceUse(globalVar);
                            delete.add(instruction);
                        }
                    }
                }
            }
        }
        for (Instruction instruction : delete) {
            instruction.remove();
        }
    }
}
