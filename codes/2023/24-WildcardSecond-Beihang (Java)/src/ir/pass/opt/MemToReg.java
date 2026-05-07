package ir.pass.opt;
import ir.Value;
import ir.instr.*;
import ir.pass.analyze.*;
import ir.value.BasicBlock;
import ir.value.ConstNumber;
import ir.value.Module;
import ir.value.user.Function;
import tools.BlockAnalyzer;
import ir.type.*;
import java.util.*;

public class MemToReg implements Pass {
    //在这里面Alloca实际上就是
    private LinkedHashMap<Alloca, Deque<Value>> reachingDef;
    //每个reachingDef保存了一个变量定义的栈
    private LinkedHashSet<BasicBlock> visited;
    public static long time0=0;

    public static double gettime(){
        long timenow = System.currentTimeMillis();
        double returntime = (timenow-time0)/1000.0;
        time0 = timenow;
        return returntime;
    }
    @Override
    public void run(Module module){
        //或许需要在DomAnalyzer中实现重构CFG(这个过程就是更新前驱后继)
        RemoveUselessUser.run(module);
        gettime();
        DomAnalyzer.calPredSuccBB(module);
        System.out.printf("mem2reg——calPredSucc: %f s%n", gettime());
        DomAnalyzer.calDomForModule(module);
        System.out.printf("mem2reg——calDomForModule: %f s%n", gettime());
        for(var function : module.functions.values()){
            if(function.blocks.size()!=0) memToRegForFunction(function);
        }
    }

    /*
        for v:variable names in origin program do
            F = {}     set of basic blocks where phi will be added
            W = {}     set of basic blocks that contains the definition of v
            for d in defs(v):
                B be the basic block containing d
                W = W union
            while w != empty:
                remove a basic block x from W
                for Y : basic block is in DF(x) do
                    if Y != F then
                        add phi for v
                        F = F union Y
                        if Y is not in defs(v) then
                            W = W union Y

    */
    public void memToRegForFunction(Function function){

        reachingDef = new LinkedHashMap<>();
        var entryBB = function.blocks.get(0);
        Iterator<Value> it = entryBB.getInsts().iterator();
        while(it.hasNext()){
            Instr instr = (Instr) it.next();
            if(instr instanceof Alloca alloca){
                //所有变量通过alloca来枚举
                if(instr.getUsers().isEmpty()) it.remove();
                if(isPromotable(alloca.getAllocType())){
                    reachingDef.put(alloca, new ArrayDeque<>());
                    var F = new LinkedList<BasicBlock>(); //set of basic blocks where phi will be added
                    var W = new LinkedList<BasicBlock>(); //set of basic blocks that contains the definition of v
                    //def is store load is use
                    for(var user:alloca.getUsers()){
                        if(user instanceof Store){
                            if(!W.contains(((Store) user).getParent()))
                                W.add(((Store)user).getParent());
                        }
                    }
                    var defs = new ArrayList<BasicBlock>(W);
                    while(!W.isEmpty()){
                        //出队
                        var BB = W.remove();
                        for(var Y : BB.getDomFrontier()){
                            if(F.contains(Y)) continue;
                            var phi = new Phi(alloca.getAllocType(),Y);
                            phi.setOwner(alloca);
                            Y.getInsts().add(0,phi);
                            F.add(Y);
                            boolean inDef = defs.contains(Y);
                            if(!inDef) W.add(Y);
                        }
                    }
                }
            }
        }

        visited = new LinkedHashSet<>();
        renamingVisitBlock(entryBB);
        for(var BB:function.blocks){
            Iterator<Value> instrIt =  BB.getInsts().iterator();
            while(instrIt.hasNext()){
                var instr = (Instr)instrIt.next();
                if(instr.isToBeDelete()){
                    instr.delete();
                    instrIt.remove();
                }
            }
        }
    }
    /*
        About how to DFS the DomTree
        In my design idominate store the node that idominated by this node
        dfs(BB father){
            if(father.idominate.size()==0) return 0
            for BB child in father.idominate:
                dfs(child)
        }
    */
    /*

    */
    public void renamingVisitBlock(BasicBlock block){
        if(visited.contains(block)) return;
        visited.add(block);
        for(var instr:block.getInsts()){
            //采取模式变量省点代码
            if(instr instanceof Alloca alloca){
                //遇到alloca直接删
                if(isPromotable(alloca.getAllocType())) alloca.setToBeDelete(true);
            }
            if(instr instanceof Load load){
                if(isPromotable(load.getType())&&load.getTarget() instanceof Alloca){
                    updateReachingDef((Alloca) load.getTarget(),block);
                    if(reachingDef.get(load.getTarget()).size()>0){
                        Instr def = (Instr) reachingDef.get(load.getTarget()).peek();
                        if(def instanceof Store)
                            load.replaceAllUsers(((Store) def).getVal());
                        else if(def instanceof Phi)
                            load.replaceAllUsers(def);
                    }else{
                        load.replaceAllUsers(getZero(load.getType()));
                    }
                    load.setToBeDelete(true);
                }
            }
            if(instr instanceof Store store){
                //更新到该变量的定义
                if(isPromotable(store.getType())&&store.getDest() instanceof  Alloca){
                    updateReachingDef((Alloca) store.getDest(),block);
                    reachingDef.get((Alloca)store.getDest()).push(store);
                    store.setToBeDelete(true);
                }
            }
            if(instr instanceof Phi phi){
                Alloca owner = ((Phi) instr).getOwner();
                updateReachingDef(owner, block);
                reachingDef.get(owner).push(phi);
            }
        }
        for(var sucBB:block.getSuccs()){
            for(Value v : sucBB.getInsts()){
                Instr instr= (Instr)v;
                if(instr instanceof Phi){
                    updateReachingDef(((Phi) instr).getOwner(), block);

                    if(reachingDef.get(((Phi) instr).getOwner()).size()>0){
                        Instr def = (Instr) reachingDef.get(((Phi) instr).getOwner()).peek();
                        if(def instanceof Store)
                            ((Phi) instr).addBlock2Value(block,((Store) def).getVal());
                        else if(def instanceof Phi)
                            ((Phi) instr).addBlock2Value(block,def);
                    }else{
                        ((Phi) instr).addBlock2Value(block,
                            getZero(((Phi) instr).getOwner().getAllocType()));
                    }
                }
            }
        }
        for(var sucBB : block.getIdominate()){
            renamingVisitBlock(sucBB);
        }
    }
    private void updateReachingDef(Alloca v,BasicBlock i){
        while(true){
            var r = reachingDef.get(v).peek();
            if(r==null||((Instr)r).getParent().getDominate().contains(i)) break;
            reachingDef.get(v).pop();
        }
    }
    private boolean isPromotable(Type type){
        return type instanceof Pointer || type instanceof IntType || type instanceof  FloatType;
    }

    private Value getZero(Type type){
        if(type instanceof Pointer){
            return new ConstNumber(0);
        }else if(type instanceof IntType){
            return new ConstNumber(0);
        }else{
            return new ConstNumber(0.0);
        }
    }
}
