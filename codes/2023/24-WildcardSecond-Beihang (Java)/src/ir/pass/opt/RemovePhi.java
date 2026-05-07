package ir.pass.opt;

import ir.Value;
import ir.instr.Br;
import ir.instr.Instr;
import ir.instr.Move;
import ir.instr.Phi;
import ir.value.BasicBlock;
import ir.value.Module;
import ir.value.user.Function;
import tools.IRBuilder;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.LinkedList;

//注意，该pass会彻底破坏SSA形式，将phi转化为来源块的尾部br前的MOVE伪指令，故必须在所有中端优化完成后后端生成前进行。
public class RemovePhi implements Pass {
    //1.第一轮，为所有block建立一个表， 遍历所有块所有phi，为phi对应来源的block加入value指向phi的表项。
    //2.第二轮，遍历block，如果自身有多个出口，则裂开为两个中间块，将自身的表分裂成对应块的表。
    //3.第三轮，此时所有有表的块都是单出口的，在末尾加入MOVE伪指令，向phi指令move
    //4.第四轮，遍历所有块的phi指令，从insts中删除phi指令。
    
    @Override
    public void run(Module module) {
        for(var func : module.functions.values()){
            removeForFunction(func);
        }
    }

    private void removeForFunction(Function function){
        if(function.blocks.size()==0){
            return;
        }
        //1.--------------------------------
        //初始化每个block的fromcopy和tocopy
        for(var block : function.blocks){
            block.fromcopy = new ArrayList<>();
            block.tocopy = new ArrayList<>();
        }
        for(var block : function.blocks){
            for(var inst : block.getInsts()){
                if(!(inst instanceof Phi)){
                    break;
                }
                for(var from : ((Phi) inst).values.keySet()){
                    from.fromcopy.add(((Phi) inst).values.get(from));
                    from.tocopy.add(inst);
                }
            }
        }
        //2.----------------------------------
        ArrayList<BasicBlock> blocks = new ArrayList<>(function.blocks);
        for(var block : blocks){
            if(block.fromcopy.size()>0 && block.getSuccs().size()>1){
                assert block.getSuccs().size()==2;
                //创建两个中间块
                BasicBlock block1 = new BasicBlock("mid1of"+block.name, function, true);
                BasicBlock block2 = new BasicBlock("mid2of"+block.name, function, true);
                //提取该块的br语句
                assert block.getInsts().get(block.getInsts().size()-1) instanceof Br;
                Br br = (Br) block.getInsts().get(block.getInsts().size()-1);
                //提取两个目标块
                BasicBlock ori1block = (BasicBlock) br.getOP(1);
                BasicBlock ori2block = (BasicBlock) br.getOP(2);
                //两个中间块跳转到原有目标
                block1.addValue(IRBuilder.buildBr(null, ori1block,null, block1));
                block2.addValue(IRBuilder.buildBr(null, ori2block, null, block2));
                //原有目标改为新加入的中间块
                br.setOperand(1, block1);
                br.setOperand(2, block2);
                //中间块加入函数
                function.insertAfter(block, block1);
                function.insertAfter(block, block2);
                //block的copy表拆分到两个中间块
                for(int i = 0;i<block.tocopy.size();i++){
                    Value from = block.fromcopy.get(i);
                    Instr to = (Instr) block.tocopy.get(i);
                    assert to.getParent() == ori1block || to.getParent() == ori2block;
                    if(to.getParent()==ori1block){
                        block1.fromcopy.add(from);
                        block1.tocopy.add(to);
                    }else{
                        block2.fromcopy.add(from);
                        block2.tocopy.add(to);
                    }
                }
                block.fromcopy.clear();
                block.tocopy.clear();
            }
        }
        //3.--------------------------------
        for(var block : function.blocks){
            if(block.fromcopy.size()>0){
                LinkedHashMap<Value, LinkedHashSet<Value>> graph = new LinkedHashMap<>();
                for(int i = 0;i<block.fromcopy.size();i++){
                    var from = block.fromcopy.get(i);
                    if(!graph.containsKey(from)){
                        graph.put(from, new LinkedHashSet<>());
                    }
                    graph.get(from).add(block.tocopy.get(i));
                }
                breakPhiLoop(graph, block);
                block.fromcopy.clear();
                block.tocopy.clear();
                for (Value value : graph.keySet()) {
                    for (Value value1 : graph.get(value)) {
                        block.fromcopy.add(value);
                        block.tocopy.add(value1);
                    }
                }
                int upperBound = block.getInsts().size()-1;
                for(int i = 0;i<block.fromcopy.size();i++){
                    LinkedList<Value> uses = new LinkedList<>();
                    uses.add(block.tocopy.get(i));
                    uses.add(block.fromcopy.get(i));
                    int pos = block.getInsts().size()-1;
                    for(int j = pos-1;j>=upperBound ;j--){
                        if(((Move) block.getInsts().get(j)).getOP(0) == block.fromcopy.get(i)){
                            pos = j;
                        }
                    }
                    block.getInsts().add(pos, new Move(uses, block));
                }
            }
        }
        //4.--------------------------------
        for(var block : function.blocks){
            ArrayList<Value> needRemove = new ArrayList<>();
            for(var inst : block.getInsts()){
                if(!(inst instanceof Phi)){
                    break;
                }
                needRemove.add(inst);
            }
            block.getInsts().removeAll(needRemove);
        }
    }

    private static void breakPhiLoop(LinkedHashMap<Value, LinkedHashSet<Value>> graph, BasicBlock block){
        boolean changed = true;
        while(changed){
            changed = false;
            ArrayList<Value> from = new ArrayList<>(graph.keySet());
            for (Value value : from) {
                LinkedHashSet<Value>visited = new LinkedHashSet<>();
                LinkedHashSet<Value> next = new LinkedHashSet<>();
                next.add(value);
                while(!next.isEmpty()){
                    var item = next.iterator().next();
                    next.remove(item);
                    if(visited.contains(item)){
                        continue;
                    }else{
                        visited.add(item);
                    }
                    if(graph.containsKey(item)&& graph.get(item).contains(value)){
                        Phi tmpphi = new Phi(item.type, block);
                        block.getInsts().add(block.getInsts().size()-1,
                            new Move(new LinkedList<>(Arrays.asList(tmpphi, item)),block));
                        graph.put(tmpphi, new LinkedHashSet<>());
                        graph.get(tmpphi).add(value);
                        graph.get(item).remove(value);
                        changed = true;
                        break;
                    }
                    if(graph.containsKey(item)){
                        for (Value value1 : graph.get(item)) {
                            if(!visited.contains(value1)){
                                next.add(value1);
                            }
                        }
                    }
                }
                if(changed){
                    break;
                }
            }
        }
    }
}
