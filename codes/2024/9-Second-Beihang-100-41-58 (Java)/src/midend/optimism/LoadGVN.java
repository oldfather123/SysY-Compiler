package midend.optimism;

import midend.BasicBlock;
import midend.Function;
import midend.Module;
import midend.Value;
import midend.instr.*;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;

public class LoadGVN {
    private Module module;
    private HashMap<Value, LoadInstr> map = new HashMap<>();

    public LoadGVN(Module module) {
        this.module = module;
    }

    public void run() {
        for (Function function : module.getFunctions()) {
            map.clear();
            GVNvisit(function.getBlockList().getFirst());
        }
    }

    public void GVNvisit(BasicBlock block) {
        map.clear();
        HashSet<LoadInstr> inserted = new HashSet<>();
        LinkedList<Instruction> temp = new LinkedList<>(block.getInstrList());
        HashMap<Value, LoadInstr> removes = new HashMap<>();
        for (Instruction instruction : temp) {
            if (instruction instanceof StoreInstr) {
                if (map.containsKey(((StoreInstr) instruction).getPointer())) {
                    removes.put(((StoreInstr) instruction).getPointer(), map.get(((StoreInstr) instruction).getPointer()));
                    map.remove(((StoreInstr) instruction).getPointer());
                    continue;
                }
            }
            if (repeated(instruction)) {
                inserted.add((LoadInstr) instruction);
            }
        }

        map.putAll(removes);

        for (BasicBlock basicBlock : block.getChildBlock()) {
            GVNvisit(basicBlock);
        }

        for (LoadInstr instruction : inserted) {
            map.remove(instruction.getValue());
        }
    }

    public boolean repeated(Instruction instruction) {
        if (instruction instanceof LoadInstr) {
            if (map.containsKey(((LoadInstr) instruction).getValue())) {
                instruction.replaceUse(map.get(((LoadInstr) instruction).getValue()));
                instruction.remove();
                return false;
            } else {
                map.put(((LoadInstr) instruction).getValue(), (LoadInstr) instruction);
                return true;
            }
        } else if (instruction instanceof StoreInstr) {
            if (map.containsKey(((StoreInstr) instruction).getPointer())) {
                map.remove(((StoreInstr) instruction).getPointer());
                return false;
            }
        } else if (instruction instanceof CallInstr) {
            map.clear();
        }
        return false;
    }

//    public void noStoreGVN(Function function) {
//        ArrayList<LoadInstr>
//        for (BasicBlock block : function.getBlockList()) {
//            for (Instruction instruction : block.getInstrList()) {
//                if (instruction instanceof LoadInstr && !map.containsKey(((LoadInstr) instruction).getValue())) {
//                    map.put(((LoadInstr) instruction).getValue(), (LoadInstr) instruction);
//                    continue;
//                }
//                //if ()
//            }
//        }
//    }
}
