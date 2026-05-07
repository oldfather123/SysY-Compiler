package backend.ir.optimize.GVN;

import backend.ir.entity.*;
import backend.ir.entity.insts.*;

import java.util.*;
import java.util.stream.BaseStream;

public class GlobalCommandMoving {

    private final Program program;

    private final boolean optimize;

    private Map<Inst,BasicBlock> earliestInstBasicBlock = new HashMap<>();

    private Map<Inst,BasicBlock> latestInstBasicBlock = new HashMap<>();

    //记录哪些basicBlock已经被处理过一次了
    private Set<BasicBlock> basicBlockSet = new HashSet<>();

    //记录哪些Inst已经被处理过了
    private Set<Inst> instVisitSet = new HashSet<>();

    private Map<BasicBlock,Integer> basicBlockDepthMap = new HashMap<>();

    private int movingNumber = 0;

    public GlobalCommandMoving(Program program,boolean optimize){
        this.program = program;this.optimize = optimize;
    }

    public void runOptimize(){
        init();
        movingEarly();
        movingLate();
        System.out.println("GCM Number: " + movingNumber);
    }

    public void init(){
        this.basicBlockSet = new HashSet<>();
        this.instVisitSet = new HashSet<>();
        this.earliestInstBasicBlock = new HashMap<>();
        this.latestInstBasicBlock = new HashMap<>();
    }

    public void movingEarly(){
        this.basicBlockSet = new HashSet<>();
        this.instVisitSet = new HashSet<>();

        for(Value value : program.getUsees()){
            if(value instanceof Function function){
                computeFunctionBasicBlockDepth(function);
                BasicBlock rootBasicBlock = function.getSuperBlock();
                movingEarlyBasicBlock(rootBasicBlock,rootBasicBlock);
            }
        }
    }

    public void movingLate(){
        this.basicBlockSet = new HashSet<>();
        this.instVisitSet = new HashSet<>();

        for(Value value : program.getUsees()){
            if(value instanceof Function function){
                computeFunctionBasicBlockDepth(function);
                List<BasicBlock> basicBlocksPostorderList = getBlockPostorderTraversal(function.getSuperBlock(),new ArrayList<>());
                for(BasicBlock block : basicBlocksPostorderList){
                    List<Inst> instList = block.getInstructions();
                    instList.reversed();
                    for(Inst inst : instList){
                        if(isGCMInstruction(inst)) scheduleLate(inst);
                    }
                }
            }
        }
    }

    public BasicBlock getDomBasicBlockLCA(BasicBlock a,BasicBlock b){
        if(a == null) return b;
        if(b == null) return a;

        while(basicBlockDepthMap.get(a) > basicBlockDepthMap.get(b)) a = a.getParentDominateBlock();
        while(basicBlockDepthMap.get(b) > basicBlockDepthMap.get(a)) b = b.getParentDominateBlock();
        while(a != b){
            a = a.getParentDominateBlock();
            b = b.getParentDominateBlock();
        }
        return b;
    }

    public List<BasicBlock> getBlockPostorderTraversal(BasicBlock basicBlock,List<BasicBlock> list){
        //System.out.println(basicBlock.getName() + " " + basicBlock.getFunction().getName());
        for(BasicBlock block : basicBlock.getChildDominateBlocks()) getBlockPostorderTraversal(block,list);
        list.add(basicBlock);return list;
    }

    public void movingEarlyBasicBlock(BasicBlock basicBlock,BasicBlock rootBasicBlock){
        if(basicBlockSet.contains(basicBlock)) return;

        basicBlockSet.add(basicBlock);
        List<Inst> blockInstList = basicBlock.getInstructions();
        for(Inst inst : blockInstList) {
            if(isGCMInstruction(inst)){
                scheduleEarly(inst,rootBasicBlock);
            }
        }
        for(BasicBlock outBasicBlock : basicBlock.getChildDominateBlocks()) movingEarlyBasicBlock(outBasicBlock,rootBasicBlock);
    }

    public void scheduleEarly(Inst inst,BasicBlock rootBasicBlock){
        if(instVisitSet.contains(inst)) return;
        if(!isGCMInstruction(inst)){
            earliestInstBasicBlock.put(inst,inst.getBasicBlock());
            return ;
        }
        instVisitSet.add(inst);
        BasicBlock earliestBlock = rootBasicBlock;
        for(Value value : inst.getUsees()){
            if(value instanceof Inst) {
                scheduleEarly((Inst) value,rootBasicBlock);
                Integer nowBasicBlockDep = basicBlockDepthMap.get(earliestBlock);
                Integer instBasicBlockDep = basicBlockDepthMap.get(earliestInstBasicBlock.get((Inst)value));
                if(instBasicBlockDep > nowBasicBlockDep) earliestBlock = earliestInstBasicBlock.get((Inst)value);
            }
        }
        earliestInstBasicBlock.put(inst,earliestBlock);
    }

    public void scheduleLate(Inst inst){
        if(instVisitSet.contains(inst)) return;
        instVisitSet.add(inst);
        BasicBlock latestBasicBlock = null;

        if(!isGCMInstruction(inst)){
            earliestInstBasicBlock.put(inst,inst.getBasicBlock());
            return ;
        }

        for(Value value : inst.getUsers()){
            if(value instanceof Inst){
                scheduleLate((Inst)value);
                BasicBlock latestUserBasicBlock = latestInstBasicBlock.get((Inst)value);
                if(value instanceof PHIInst){
                    for(int index = 0;index < ((PHIInst) value).getUsees().size();index ++ ){
                        if(((PHIInst) value).getUsees().get(index).getName().equals(inst.getName())){
                            latestBasicBlock = getDomBasicBlockLCA(latestBasicBlock,((PHIInst) value).getPreBlocks().get(index));
                        }
                    }
                }
                latestBasicBlock = getDomBasicBlockLCA(latestBasicBlock,latestInstBasicBlock.get((Inst)value));
            }
        }
        latestInstBasicBlock.put(inst,latestBasicBlock);

        BasicBlock earliestBasicBlock = earliestInstBasicBlock.get(inst);
        Integer latestDepth = basicBlockDepthMap.get(latestBasicBlock),earliestDepth = basicBlockDepthMap.get(earliestBasicBlock);
        if(latestBasicBlock == null || earliestBasicBlock == null) return;
        if(latestDepth < earliestDepth){
            System.err.println("GCM is a big JB!!");return;
        }
        scheduleInst(earliestBasicBlock,latestBasicBlock,inst);
    }

    public void scheduleInst(BasicBlock earliestBlock,BasicBlock latestBlock,Inst inst){
        BasicBlock nowBestBlock  = latestBlock;
        BasicBlock nowPlaceBlock = latestBlock;
        while(nowPlaceBlock != earliestBlock){
            int loopDeepNow = nowPlaceBlock.getLoopDepth();
            int bestDeepNow = nowBestBlock.getLoopDepth();
            if(loopDeepNow < bestDeepNow) nowBestBlock = nowPlaceBlock;
            else if(loopDeepNow == bestDeepNow){
                int domainDepth = basicBlockDepthMap.get(nowPlaceBlock);
                int bestBlockDepth = basicBlockDepthMap.get(nowBestBlock);
                if(domainDepth > bestBlockDepth) nowBestBlock = nowPlaceBlock;
            }
            nowPlaceBlock = nowPlaceBlock.getParentDominateBlock();
        }

        if(nowBestBlock != inst.getBasicBlock()) insertInstInBasicBlock(inst,nowBestBlock);
    }

    public void insertInstInBasicBlock(Inst inst,BasicBlock bestBasicBlock){
        Set<Value> userValueSet = new HashSet<>(inst.getUsers());
        movingNumber ++;

        for(int index = 0;index < bestBasicBlock.getUsees().size();index ++ ){
            Value value = bestBasicBlock.getUsee(index);
            if(value instanceof PHIInst) continue;
            if(userValueSet.contains(value)){
                bestBasicBlock.getUsees().add(index,inst);
                inst.getBasicBlock().getUsees().remove(inst);inst.setBasicBlock(bestBasicBlock);
                return;
            }
        }
        bestBasicBlock.getUsees().addLast(inst);
        inst.getBasicBlock().getUsees().remove(inst);inst.setBasicBlock(bestBasicBlock);
    }


    public boolean isGCMInstruction(Inst inst){
        if(inst instanceof PHIInst || inst instanceof PCopyInst || inst instanceof UCJumpInst
            || inst instanceof BranchInst || inst instanceof RetInst || inst instanceof StoreInst || inst instanceof LoadInst
            || inst instanceof InputInst || inst instanceof OutputInst || inst instanceof AllocateInst){
            return false;
        }
        else if(inst instanceof CallInst){
            if(inst.getUsee(0) instanceof Function){
                Function function = (Function) inst.getUsees().getFirst();
                return function.isGVNInstruction();
            }
            else return false;
        }
        else return true;
    }

    public void computeFunctionBasicBlockDepth(Function function){
       List<BasicBlock> basicBlocksList = new LinkedList<>();
       Set<BasicBlock> basicBlockMeetSet = new HashSet<>();
       basicBlockDepthMap = new HashMap<>();

       basicBlockDepthMap.put(function.getSuperBlock(),0);
       basicBlocksList.addFirst(function.getSuperBlock());
       basicBlockMeetSet.add(function.getSuperBlock());

       while(!basicBlocksList.isEmpty()) {
           BasicBlock basicBlock = basicBlocksList.removeLast();

           Integer basicBlockDepth = basicBlockDepthMap.get(basicBlock);
           for (BasicBlock outBasicBlock : basicBlock.getChildDominateBlocks()) {
               if (!basicBlockMeetSet.contains(outBasicBlock)) {
                   basicBlockMeetSet.add(basicBlock);
                   basicBlockDepthMap.put(outBasicBlock, basicBlockDepth + 1);
                   basicBlocksList.addFirst(outBasicBlock);
               }
           }
       }
    }
}
