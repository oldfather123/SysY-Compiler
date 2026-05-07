package midend.optimism;

import midend.Function;
import midend.Module;
import midend.Param;
import midend.User;
import midend.instr.AllocaInstr;
import midend.instr.Instruction;
import midend.instr.LoadInstr;
import midend.instr.StoreInstr;

import java.util.ArrayList;

public class RemoveArgStoreLoad {
    private Module module;

    public RemoveArgStoreLoad(Module module) {
        this.module = module;
    }

    public void run() {
        for (Function function : module.getFunctions()) {
            for (Param param : function.getParams()) {
                StoreInstr storeInstr = null;
                boolean only = true;
                for (User user : param.getUserList()) {
                    if (user instanceof StoreInstr && ((StoreInstr) user).getPointer() instanceof AllocaInstr) {
                        if (storeInstr != null) {
                            only = false;
                        }
                        storeInstr = (StoreInstr) user;
                    }
                }
                if (!only || storeInstr == null) {
                    continue;
                }

                AllocaInstr allocaInstr = (AllocaInstr) storeInstr.getPointer();
                ArrayList<Instruction> instructions = new ArrayList<>();
                for (User user : allocaInstr.getUserList()) {
                    if (user instanceof LoadInstr) {
                        user.replaceUse(param);
                        instructions.add((Instruction) user);
                    }
                }
                instructions.add(storeInstr);
                instructions.add(allocaInstr);
                for (Instruction instruction : instructions) {
                    instruction.remove();
                }
            }
        }
    }
}
