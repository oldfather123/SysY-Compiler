package midend.optimism;

import midend.*;
import midend.LLVMType.ArrayType;
import midend.LLVMType.IntegerType;
import midend.LLVMType.PointerType;
import midend.LLVMType.Type;
import midend.Module;
import midend.instr.GetElementPtrInstr;
import midend.instr.Instruction;

import java.util.ArrayList;

public class GepSeparate {
    private Module module;

    public GepSeparate(Module module) {
        this.module = module;
    }

    public void run() {
        for (Function function : module.getFunctions()) {
            for (BasicBlock block : function.getBlockList()) {
                ArrayList<Instruction> delete = new ArrayList<>();
                for (int count0 = 0; count0 < block.getInstrList().size(); count0++) {
                    Instruction instruction = block.getInstrList().get(count0);
                    if (instruction instanceof GetElementPtrInstr) {
                        if (((GetElementPtrInstr) instruction).getIndexes().size() >= 2) {
                            ArrayList<Instruction> instructions = new ArrayList<>();
                            ArrayList<Value> values = new ArrayList<>();
                            if (((GetElementPtrInstr) instruction).getIndexes().size() == 2 && ((GetElementPtrInstr) instruction).getIndexes().get(0) instanceof ConstInt) {
                                continue;
                            }
                            values.add(((GetElementPtrInstr) instruction).getIndexes().get(0));
                            GetElementPtrInstr getElementPtrInstr1 = getElementPtrInstr(values, ((GetElementPtrInstr) instruction).getTarget());
                            getElementPtrInstr1.setBasicBlock(block);
                            instructions.add(getElementPtrInstr1);
                            for (int count = 1; count < ((GetElementPtrInstr) instruction).getIndexes().size(); count++) {
                                ArrayList<Value> values1 = new ArrayList<>();
                                values1.add(new ConstInt(IntegerType.i32, 0));
                                values1.add(((GetElementPtrInstr) instruction).getIndexes().get(count));
                                GetElementPtrInstr getElementPtrInstr = getElementPtrInstr(values1, instructions.get(count - 1));
                                getElementPtrInstr.setBasicBlock(block);
                                instructions.add(getElementPtrInstr);
                            }
                            int index = block.getInstrList().indexOf(instruction);
                            for (int count = instructions.size() - 1; count >= 0; count--) {
                                block.getInstrList().add(index, instructions.get(count));
                            }
                            instruction.replaceUse(instructions.get(instructions.size() - 1));
                            delete.add(instruction);
                            count0 += instructions.size();
                        }
                    }
                    for (Instruction instruction1 : delete) {
                        instruction1.remove();
                    }
                }
            }

            for (BasicBlock block : function.getBlockList()) {
                ArrayList<Instruction> delete = new ArrayList<>();
                for (Instruction instruction : block.getInstrList()) {
                    if (instruction instanceof GetElementPtrInstr && ((GetElementPtrInstr) instruction).getIndexes().size() == 1 && ((GetElementPtrInstr) instruction).getIndexes().get(0) instanceof ConstInt && ((ConstInt) ((GetElementPtrInstr) instruction).getIndexes().get(0)).getValue() == 0) {
                        instruction.replaceUse(((GetElementPtrInstr) instruction).getTarget());
                        delete.add(instruction);
                    }
                }
                for (Instruction instruction : delete) {
                    instruction.remove();
                }
            }
        }
    }

    public GetElementPtrInstr getElementPtrInstr(ArrayList<Value> indexs, Value ptrval) {
        Type type = ((PointerType) ptrval.getType()).getElementType();
        for (int count = 0; count < indexs.size() - 1; count++) {
            type = ((ArrayType) type).getElementType();
        }
        type = new PointerType(type);
        GetElementPtrInstr getElementPtrInstr = new GetElementPtrInstr(type, ptrval, indexs);
        return getElementPtrInstr;
    }
}
