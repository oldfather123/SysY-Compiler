package midend.optimism;

import midend.*;
import midend.Module;
import midend.instr.*;

import java.util.ArrayList;

public class PeepHole {
    private Module module;

    public PeepHole(Module module) {
        this.module = module;
    }

    public void run() {
        for (Function function : module.getFunctions()) {
            simplifyLoad(function);
        }
        ArrayInit();
        AssignToMemSet();
    }

    public void AssignToMemSet() {

    }

    public void ArrayInit() {
        for (GlobalVar globalVar : module.getGlobalVars()) {
            if (globalVar.isArray()) {
                ArrayList<GetElementPtrInstr> geps = new ArrayList<>();
                for (User user : globalVar.getUserList()) {
                    if (user instanceof GetElementPtrInstr) {
                        geps.add((GetElementPtrInstr) user);
                    }
                }
                ArrayList<StoreInstr> storeInstrs = new ArrayList<>();
                for (GetElementPtrInstr gep : geps) {
                    for (User user : gep.getUserList()) {
                        if (user instanceof StoreInstr) {
                            storeInstrs.add((StoreInstr) user);
                        }
                    }
                }
                if (storeInstrs.size() == 1) {
                    StoreInstr storeInstr = storeInstrs.get(0);
                    if (storeInstr.getBasicBlock().getParentFunc().getName().equals("@main")) {
                        if (storeInstr.getValue() instanceof ConstInt && ((ConstInt) storeInstr.getValue()).getValue() == 0) {
                            storeInstr.remove();
                        }
                    }
                }
            }
        }
    }

    public void simplifyLoad(Function function) {
        ArrayList<Instruction> instructions = new ArrayList<>();
        for (BasicBlock block : function.getBlockList()) {
            instructions.clear();
            for (Instruction instruction : block.getInstrList()) {
                if (instruction instanceof StoreInstr || instruction instanceof LoadInstr || instruction instanceof CallInstr) {
                    instructions.add(instruction);
                }
            }
            for (int count = 0; count < instructions.size() - 1; count++) {
                Instruction instruction = instructions.get(count);
                int temp = count;
                Instruction after = instructions.get(++temp);
                if (instruction instanceof StoreInstr) {
                    while (temp < instructions.size() && !(after instanceof StoreInstr && ((StoreInstr) after).getPointer() == ((StoreInstr) instruction).getPointer())) {
                        if (after instanceof CallInstr && after.getValueList().contains(((StoreInstr) instruction).getPointer())) {
                            break;
                        }
                        if (after instanceof LoadInstr && ((LoadInstr) after).getValue() == ((StoreInstr) instruction).getPointer()) {
                            after.replaceUse(((StoreInstr) instruction).getValue());
                            after.remove();
                        }
                        if (temp == instructions.size() - 1) {
                            break;
                        }
                        after = instructions.get(++temp);
                    }
                }
            }
        }
    }
}
