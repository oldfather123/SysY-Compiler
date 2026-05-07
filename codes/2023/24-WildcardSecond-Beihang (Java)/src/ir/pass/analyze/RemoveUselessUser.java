package ir.pass.analyze;

import ir.Value;
import ir.instr.Instr;
import ir.instr.Phi;
import ir.value.BasicBlock;
import ir.value.Module;
import ir.value.User;

import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.LinkedList;

public class RemoveUselessUser {
    public static void run(Module module){
        LinkedHashSet<Value> users = new LinkedHashSet<Value>();
        for(var func : module.functions.values()){
            for(var block : func.blocks){
                users.add(block);
                block.users = new LinkedList<>();
                block.users.add(func);
                block.function = func;
                users.addAll(block.getInsts());
            }
        }
        for(var func : module.functions.values()){
            for(var block : func.blocks){
                for(var inst : block.getInsts()){
                    for(var used : ((Instr)inst).getOperands()){
                        used.addUser((User) inst);
                    }
                    ArrayList<User> needremove = new ArrayList<>();
                    for(var user : inst.users){
                        if(!users.contains(user)){
                            needremove.add(user);
                        }
                    }
                    inst.users.removeAll(needremove);
                    if(inst instanceof Phi){
                        for (BasicBlock basicBlock : ((Phi) inst).getValues().keySet()) {
                            basicBlock.users.add((User) inst);
                        }
                    }
                }
            }
        }
        for(var inst : module.init.getInsts()){
            for(var used : ((Instr)inst).getOperands()){
                used.addUser((User) inst);
            }
            ArrayList<User> needremove = new ArrayList<>();
            for(var user : inst.users){
                if(!users.contains(user)){
                    needremove.add(user);
                }
            }
            inst.users.removeAll(needremove);
        }

    }
}
