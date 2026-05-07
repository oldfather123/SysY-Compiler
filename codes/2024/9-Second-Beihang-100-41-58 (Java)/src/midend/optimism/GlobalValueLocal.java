package midend.optimism;

import midend.*;
import midend.LLVMType.PointerType;
import midend.Module;
import midend.instr.AllocaInstr;
import midend.instr.LoadInstr;
import midend.instr.StoreInstr;

import java.util.ArrayList;
import java.util.HashSet;

public class GlobalValueLocal {
    private Module module;
    private IRBuilder builder = new IRBuilder();

    public GlobalValueLocal(Module module) {
        this.module = module;
    }

    public void run() {
        ArrayList<GlobalVar> deleteVar = new ArrayList<>();
        for (GlobalVar globalVar : module.getGlobalVars()) {
            if (globalVar.isArray()) {
                continue;
            }
            if(local(globalVar)) {
                deleteVar.add(globalVar);
            }
        }

        for (GlobalVar delete : deleteVar) {
            module.getGlobalVars().remove(delete);
        }
    }

    public boolean local(GlobalVar globalVar) {
        HashSet<Function> useFunc = new HashSet<>();
        ArrayList<Function> functions = new ArrayList<>();
        ArrayList<StoreInstr> storeInstrs = new ArrayList<>();
        ArrayList<LoadInstr> loadInstrs = new ArrayList<>();
        for (User user : globalVar.getUserList()) {
            if (user instanceof StoreInstr) {
                if (!useFunc.contains(((StoreInstr) user).getBasicBlock().getParentFunc())) {
                    useFunc.add(((StoreInstr) user).getBasicBlock().getParentFunc());
                    functions.add(((StoreInstr) user).getBasicBlock().getParentFunc());
                }
                storeInstrs.add((StoreInstr) user);
            } else if (user instanceof LoadInstr) {
                if (!useFunc.contains(((LoadInstr) user).getBasicBlock().getParentFunc())) {
                    useFunc.add(((LoadInstr) user).getBasicBlock().getParentFunc());
                    functions.add(((LoadInstr) user).getBasicBlock().getParentFunc());
                }
                loadInstrs.add((LoadInstr) user);
            }
        }

        if (useFunc.isEmpty()) { //没有被用到的全局变量
            return true;
        }

        if (storeInstrs.isEmpty()) { //没有被修改过
            for (LoadInstr loadInstr : loadInstrs) {
                loadInstr.replaceUse(globalVar.getValue());
                loadInstr.remove();
            }
            return true;
        }

        if (useFunc.size() == 1) { //只在一个函数被使用
            Function function = functions.get(0);
            if (!function.getName().equals("@main")) {
                return false;
            }
            BasicBlock block = function.getBlockList().get(0);
            AllocaInstr allocaInstr = builder.buildAllocaInstr(((PointerType) globalVar.getType()).getElementType(), block);
            StoreInstr storeInstr = builder.buildStoreInstr(globalVar.getValue(), allocaInstr, block);
            block.getInstrList().addFirst(storeInstr);
            block.getInstrList().removeLast();
            block.getInstrList().addFirst(allocaInstr);
            globalVar.replaceUse(allocaInstr);
            return true;
        }
        return false;
    }
}
