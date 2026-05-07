package ir.pass.opt;

import ir.Value;
import ir.instr.BinaryInstr;
import ir.instr.Br;
import ir.instr.GetElementPtrInst;
import ir.instr.Instr;
import ir.instr.Phi;
import ir.pass.analyze.DomAnalyzer;
import ir.pass.analyze.RemoveUselessUser;
import ir.value.BasicBlock;
import ir.value.Module;
import ir.value.User;
import tools.BlockAnalyzer;
import tools.ModuleFlattener;

import java.util.ArrayList;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.LinkedList;
import java.util.Stack;

public class BrRemoving implements Pass {
    public boolean changedCFG = false;

    public void runStrong(Module module){
        run(module);
        BlockAnalyzer.updateBlockPredAndSuccs(module);
        for (var func : module.functions.values()){
            LinkedHashMap<Value, Value> needReplace = new LinkedHashMap<>();
            for(var block : func.blocks){
                if(block.getInsts().size()==1){
                    if(!(block.getInsts().get(0) instanceof Br keyBr)){
                        continue;
                    }
                    boolean canChange = true;
                    for(var user : block.users){
                        if(user instanceof Phi){
                            canChange = false;
                            break;
                        }
                    }
                    if(!canChange) continue;
                    var targetBlock = keyBr.getOP(0);
                    if(keyBr.getOperands().size()==1){
                        needReplace.put(block, targetBlock);
                        var allUsers = new ArrayList<User>(block.users);
                        for(var user : allUsers){
                            if(user instanceof Br){
                                user.replaceOp(block, targetBlock);
                                if(user.getOperands().size()>1 && user.getOP(1).equals(user.getOP(2))){
                                    user.getOP(0).users.remove(user);
                                    user.getOperands().remove(0);
                                    user.getOperands().remove(1);
                                }
                            }
                        }
                        keyBr.delete();
                    }else{
                        var allUsers = new ArrayList<User>(block.users);
                        for(var user : allUsers){
                            if(user instanceof Br && user.getOperands().size()==1){
                                LinkedList<Value> uses = new LinkedList<>(keyBr.getOperands());
                                Br newBr = new Br(uses, ((Br) user).getParent());
                                ((Br) user).getParent().getInsts().remove(
                                    ((Br) user).getParent().getInsts().size()-1
                                );
                                ((Br) user).getParent().getInsts().add(newBr);
                                user.delete();
                                block.replaceAllUsers(targetBlock);
                            }
                        }
                    }
                }
            }
            for(var keyBlock : needReplace.keySet()){
                func.blocks.remove(keyBlock);
            }
        }
        BlockAnalyzer.updateBlockPredAndSuccs(module);
        for (var func : module.functions.values()) {
            if(func.blocks.size()==0){
                continue;
            }
            LinkedHashSet<BasicBlock> access = new LinkedHashSet<>();
            LinkedHashSet<BasicBlock> visitedClose = new LinkedHashSet<>();
            Stack<BasicBlock> visiting = new Stack<>();
            visiting.add(func.blocks.get(0));
            while(visiting.size()>0){
                BasicBlock tmp = visiting.pop();
                if(visitedClose.contains(tmp)){
                    continue;
                }
                visitedClose.add(tmp);
                access.add(tmp);
                visiting.addAll(tmp.getSuccs());
            }

            boolean changed = true;
            while (changed) {
                BlockAnalyzer.updateBlockPredAndSuccs(module);
//                System.out.println(func);
                changed = false;
                ArrayList<BasicBlock> blocks = new ArrayList<>(func.blocks);
                if (blocks.isEmpty()) {
                    break;
                }
                BasicBlock entry = func.blocks.get(0);
                LinkedHashMap<BasicBlock, BasicBlock> visited = new LinkedHashMap<>();
                for (int i = blocks.size() - 1; i >= 0; i--) {
                    var block = blocks.get(i);
                    while (visited.containsKey(block)) {
                        block = visited.get(block);
                    }
                    if (block.getSuccs().size() == 1 &&
                        block.getSuccs().get(0).getPreds().size() == 1 && !block.getSuccs().get(0).equals(entry)) {
                        changed = true;
                        visited.put(block.getSuccs().get(0), block);
                        BasicBlock merged = block.getSuccs().get(0);
                        ((User) block.getInsts().get(block.getInsts().size() - 1)).delete();
                        block.getInsts().remove(block.getInsts().size() - 1);
                        for (var ins : merged.getInsts()) {
                            ((Instr) ins).setParent(block);
                            block.addValue(ins);
                        }
                        for(var checkBlock : blocks){
                            for(var ins : checkBlock.getInsts()){
                                if(ins instanceof Phi){
                                    ((Phi) ins).replaceBlockWith(merged, block);
                                }
                            }
                        }
                        func.blocks.remove(merged);
                    }
                    if (!access.contains(block)) {
                        changedCFG = true;
                        func.blocks.remove(block);
                        for(var inst : block.getInsts()){
                            if(inst instanceof BinaryInstr || inst instanceof GetElementPtrInst){
                                entry.getInsts().add(entry.getInsts().size()-1, inst);
                            }else {
                                ((Instr) inst).delete();
                            }
                        }
                        changed = true;
                        for(var checkBlock : blocks){
                            ArrayList<Phi> needDelete = new ArrayList<>();
                            for(var inst: checkBlock.getInsts()){
                                if(inst instanceof Phi){
                                    ((Phi) inst).checkAndRemoveBlock(block);
                                    if(((Phi) inst).values.size()==1){
                                        needDelete.add((Phi) inst);
                                    }
                                }
                            }
                            for(Phi needDeletePhi : needDelete){
                                Value commingvalue = null;
                                for(Value tmpvalue : ((Phi) needDeletePhi).values.values()){
                                    commingvalue = tmpvalue;
                                }
                                needDeletePhi.replaceAllUsers(commingvalue);
                                ((Phi) needDeletePhi).delete();
                                checkBlock.getInsts().remove(needDeletePhi);
                            }
                        }
                    }
                }
            }
        }
    }
    @Override
    public void run(Module module) {
        BlockAnalyzer.updateBlockPredAndSuccs(module);
        RemoveUselessUser.run(module);
        for (var func : module.functions.values()) {
            if(func.blocks.size()==0){
                continue;
            }
            LinkedHashSet<BasicBlock> access = new LinkedHashSet<BasicBlock>();
            LinkedHashSet<BasicBlock> visitedClose = new LinkedHashSet<BasicBlock>();
            Stack<BasicBlock> visiting = new Stack<>();
            visiting.add(func.blocks.get(0));
            while(visiting.size()>0){
                BasicBlock tmp = visiting.pop();
                if(visitedClose.contains(tmp)){
                    continue;
                }
                visitedClose.add(tmp);
                access.add(tmp);
                visiting.addAll(tmp.getSuccs());
            }

            boolean changed = true;
            while (changed) {
                BlockAnalyzer.updateBlockPredAndSuccs(module);
//                System.out.println(func);
                changed = false;
                ArrayList<BasicBlock> blocks = new ArrayList<>(func.blocks);
                if (blocks.isEmpty()) {
                    break;
                }
                BasicBlock entry = func.blocks.get(0);
                LinkedHashMap<BasicBlock, BasicBlock> visited = new LinkedHashMap<>();
                for (int i = blocks.size() - 1; i >= 0; i--) {
                    var block = blocks.get(i);
                    while (visited.containsKey(block)) {
                        block = visited.get(block);
                    }
                    if (block.getSuccs().size() == 1 &&
                        block.getSuccs().get(0).getPreds().size() == 1) {
                        changed = true;
                        visited.put(block.getSuccs().get(0), block);
                        BasicBlock merged = block.getSuccs().get(0);
                        ((User) block.getInsts().get(block.getInsts().size() - 1)).delete();
                        block.getInsts().remove(block.getInsts().size() - 1);
                        for (var ins : merged.getInsts()) {
                            ((Instr) ins).setParent(block);
                            block.addValue(ins);
                        }
                        for(var checkBlock : blocks){
                            for(var ins : checkBlock.getInsts()){
                                if(ins instanceof Phi){
                                    ((Phi) ins).replaceBlockWith(merged, block);
                                }
                            }
                        }
                        func.blocks.remove(merged);
                    }
                    if (!access.contains(block)) {
                        changedCFG = true;
                        func.blocks.remove(block);
                        for(var inst : block.getInsts()){
                            if(inst instanceof BinaryInstr || inst instanceof GetElementPtrInst){
                                entry.getInsts().add(entry.getInsts().size()-1, inst);
                            }else {
                                ((Instr) inst).delete();
                            }
                        }
                        changed = true;
                        for(var checkBlock : blocks){
                            ArrayList<Phi> needDelete = new ArrayList<>();
                            for(var inst: checkBlock.getInsts()){
                                if(inst instanceof Phi){
                                    ((Phi) inst).checkAndRemoveBlock(block);
                                    if(((Phi) inst).values.size()==1){
                                        needDelete.add((Phi) inst);
                                    }
                                }
                            }
                            for(Phi needDeletePhi : needDelete){
                                Value commingvalue = null;
                                for(Value tmpvalue : ((Phi) needDeletePhi).values.values()){
                                    commingvalue = tmpvalue;
                                }
                                needDeletePhi.replaceAllUsers(commingvalue);
                                ((Phi) needDeletePhi).delete();
                                checkBlock.getInsts().remove(needDeletePhi);
                            }
                        }
                    }
                }
            }
        }
    }
}
