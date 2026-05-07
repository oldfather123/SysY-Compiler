package midend.optimism;

import midend.*;
import midend.Module;
import midend.analysis.AliasAnalysis;
import midend.instr.*;

import java.util.ArrayList;
import java.util.HashMap;

public class RemoveUseLessStore {
    private Module module;

    public RemoveUseLessStore(Module module) {
        this.module = module;
    }

    public void run() {
//        for (Function function : module.getFunctions()) {
//            removeExcessiveStore(function);
//            removeUseLessStore(function);
//            removeNotUsedStore(function);
//        }
        if (module.getFunctions().size() == 1) {
            removeNotUsedStore(module.getFunctions().get(0));
        }
    }

    public void removeExcessiveStore(Function function) {
        HashMap<Value, StoreInstr> storeMap = new HashMap<>();
        ArrayList<Instruction> delete = new ArrayList<>();
        for (BasicBlock block : function.getBlockList()) {
            for (Instruction instruction : block.getInstrList()) {
                if (instruction instanceof StoreInstr) {
                    Value pointer = ((StoreInstr) instruction).getPointer();
                    if (!storeMap.containsKey(pointer)) {
                        storeMap.put(pointer, (StoreInstr) instruction);
                    } else {
                        delete.add(storeMap.get(pointer));
                        storeMap.replace(pointer, (StoreInstr) instruction);
                    }
                } else {
                    if (instruction instanceof LoadInstr || instruction instanceof GetElementPtrInstr || (instruction instanceof CallInstr && (((CallInstr) instruction).getFunction().isSideEffect() || ((CallInstr) instruction).getFunction().isRecursive()))) {
                        storeMap.clear();
                    } else if (instruction instanceof CallInstr) {
                        for (Value value : ((CallInstr) instruction).getValues()) {
                            if (value.getType().isPointer()) {
                                storeMap.clear();
                            }
                        }
                    }
                }
            }
        }
        for (Instruction instruction : delete) {
            instruction.remove();
        }
    }

    public void removeUseLessStore(Function function) {
        ArrayList<Instruction> memoryInstr = new ArrayList<>();
        for (BasicBlock block : function.getBlockList()) {
            for (Instruction instruction : block.getInstrList()) {
                if (instruction instanceof StoreInstr || instruction instanceof LoadInstr) {
                    memoryInstr.add(instruction);
                }
            }
        }
        ArrayList<Instruction> deletes = new ArrayList<>();
        for (int count = 0; count < memoryInstr.size() - 1; count++) {
            Instruction instruction = memoryInstr.get(count);
            if (instruction instanceof StoreInstr) {
                for (int count1 = count + 1; count1 < memoryInstr.size(); count1++) {
                    Instruction after = memoryInstr.get(count1);
                    if (after instanceof LoadInstr && AliasAnalysis.isSame(((LoadInstr) after).getValue(), ((StoreInstr) instruction).getPointer())) {
                        break;
                    } else if (after instanceof StoreInstr && AliasAnalysis.isSame(((StoreInstr) after).getPointer(), ((StoreInstr) instruction).getPointer())) {
                        //常数则可以省略
                        if (!(AliasAnalysis.hasReg(((StoreInstr) after).getPointer())
                                && AliasAnalysis.hasReg(((StoreInstr) instruction).getPointer()))) {
                            deletes.add(instruction);
                            break;
                        } else { //寄存器需谨慎，或者不应省略
                            break;
                        }
                    }
                }
            }
        }
        for (Instruction delete : deletes) {
            delete.remove();
        }
    }

    public void removeNotUsedStore(Function function) {
        ArrayList<Instruction> memInstr = new ArrayList<>();
        ArrayList<Instruction> delete = new ArrayList<>();
        for (BasicBlock block : function.getBlockList()) {
            for (Instruction instruction : block.getInstrList()) {
                if (instruction instanceof GetElementPtrInstr) {
                    memInstr.add(instruction);
                }
                if (instruction instanceof LoadInstr) {
                    return;
                }
            }
        }
        for (Instruction instruction : memInstr) {
            boolean notUsed = true;
            for (User user : instruction.getUserList()) {
                if (user instanceof LoadInstr) {
                    notUsed = false;
                    break;
                }
            }
            if (notUsed) {
                delete.add(instruction);
                for (User user : instruction.getUserList()) {
                    if (user instanceof Instruction) {
                        delete.add((Instruction) user);
                    }
                }
            }
        }
        for (Instruction instruction : delete) {
            instruction.remove();
        }
    }
}
