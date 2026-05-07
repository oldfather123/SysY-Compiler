package midend.optimism;

import midend.*;
import midend.LLVMType.IntegerType;
import midend.Module;
import midend.instr.GetElementPtrInstr;
import midend.instr.Instruction;

import java.util.ArrayList;

public class GepFuse {
    private Module module;

    public GepFuse(Module module) {
        this.module = module;
    }

    public void run() {
        for (Function function : module.getFunctions()) {
            for (BasicBlock block : function.getBlockList()) {
                for (int count0 = 0; count0 < block.getInstrList().size(); count0++) {
                    Instruction instruction = block.getInstrList().get(count0);
                    if (instruction instanceof GetElementPtrInstr && ((GetElementPtrInstr) instruction).getTarget() instanceof GetElementPtrInstr) {
                        if (((GetElementPtrInstr) instruction).getIndexes().get(0) instanceof ConstInt && ((GetElementPtrInstr) ((GetElementPtrInstr) instruction).getTarget()).getIndexes().get(((GetElementPtrInstr) ((GetElementPtrInstr) instruction).getTarget()).getIndexes().size() - 1) instanceof ConstInt) {
                            ArrayList<Value> indexes = new ArrayList<>();
                            for (int count = 0; count < ((GetElementPtrInstr) ((GetElementPtrInstr) instruction).getTarget()).getIndexes().size() - 1; count++) {
                                indexes.add(((GetElementPtrInstr) ((GetElementPtrInstr) instruction).getTarget()).getIndexes().get(count));
                            }
                            indexes.add(new ConstInt(IntegerType.i32, ((ConstInt) ((GetElementPtrInstr) instruction).getIndexes().get(0)).getValue() + ((ConstInt) ((GetElementPtrInstr) ((GetElementPtrInstr) instruction).getTarget()).getIndexes().get(((GetElementPtrInstr) ((GetElementPtrInstr) instruction).getTarget()).getIndexes().size() - 1)).getValue()));
                            for (int count = 1; count < ((GetElementPtrInstr) instruction).getIndexes().size(); count++) {
                                indexes.add(((GetElementPtrInstr) instruction).getIndexes().get(count));
                            }
                            GetElementPtrInstr newGep = new GetElementPtrInstr(instruction.getType(), ((GetElementPtrInstr) ((GetElementPtrInstr) instruction).getTarget()).getTarget(), indexes);
                            newGep.setBasicBlock(block);
                            block.getInstrList().add(count0, newGep);
                            instruction.replaceUse(newGep);
                            instruction.remove();
                        }
                    }
                }
            }
        }
    }
}
