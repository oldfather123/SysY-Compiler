package Pass.IR;

import IR.IRModule;
import IR.Value.BasicBlock;
import IR.Value.Function;
import IR.Value.Instructions.*;
import IR.Value.User;
import IR.Value.Value;
import Pass.IR.Utils.IRLoop;
import Pass.IR.Utils.LoopAnalysis;
import Pass.Pass;
import Utils.DataStruct.IList;
import Utils.DataStruct.Pair;

import java.util.ArrayList;
import java.util.HashSet;

public class LoopFusion implements Pass.IRPass {
    private Value itVar;
    private Value itVar2;

    @Override
    public void run(IRModule module) {
        for(Function function : module.getFunctions()){
            LoopAnalysis.runLoopInfo(function);
            LoopAnalysis.runLoopIndvarInfo(function);

            ArrayList<Pair<IRLoop, IRLoop>> fusionLoops = new ArrayList<>();
            for(IRLoop loop : function.getAllLoops()){
                for(IRLoop loop2 : function.getAllLoops()){
                    if(canFusion(loop, loop2)){
                        fusionLoops.add(new Pair<>(loop, loop2));
                    }
                }
            }

            for(Pair<IRLoop, IRLoop> loopPair : fusionLoops){
                fusion(loopPair.getFirst(), loopPair.getSecond());
            }
        }
    }

    private void fusion(IRLoop loop, IRLoop loop2){
        BrInst brInst = (BrInst) loop.getHead().getLastInst();
        BrInst brInst2 = (BrInst) loop2.getHead().getLastInst();
        BasicBlock mainBb = brInst.getTrueBlock();
        BasicBlock mainBb2 = brInst2.getTrueBlock();
        Instruction insertPos = (Instruction) loop.getItAlu();

        HashSet<Instruction> notInsertInsts = new HashSet<>();
        notInsertInsts.add(mainBb2.getLastInst());
        notInsertInsts.add((Instruction) loop2.getItAlu());

        ArrayList<Instruction> moveInsts = new ArrayList<>();
        for(IList.INode<Instruction, BasicBlock> instNode : mainBb2.getInsts()){
            Instruction inst = instNode.getValue();
            if(!notInsertInsts.contains(inst)){
                moveInsts.add(inst);
            }
        }
        for(Instruction moveInst : moveInsts){
            moveInst.removeFromBb();
            moveInst.insertBefore(insertPos);
        }

        Value itVar = loop.getItVar();
        Value itVar2 = loop2.getItVar();
        itVar2.replaceUsedWith(itVar);

    }

    private boolean canFusion(IRLoop loop, IRLoop loop2){
        if(loop.getSubLoops().size() != 0 || loop2.getSubLoops().size() != 0){
            return false;
        }
        if(loop.equals(loop2)){
            return false;
        }
        if(!loop.isSimpleLoop() || !loop2.isSimpleLoop()
                || !loop.isSetIndVar() || !loop2.isSetIndVar()){
            return false;
        }
        BasicBlock head = loop.getHead();
        BasicBlock head2 = loop2.getHead();
        BasicBlock exitBlock = null;
        BasicBlock exitBlock2 = null;
        for(BasicBlock exit : loop.getExitBlocks()){
            exitBlock = exit;
        }
        for(BasicBlock exit : loop2.getExitBlocks()){
            exitBlock2 = exit;
        }

        if(exitBlock == null || exitBlock2 == null) return false;
        if(!(exitBlock.getNxtBlocks().size() == 1
                && exitBlock.getNxtBlocks().get(0).equals(head2))){
            return false;
        }

        for(IList.INode<Instruction, BasicBlock> exitInstNode : exitBlock.getInsts()){
            if(exitInstNode.getValue() instanceof CallInst){
                return false;
            }
        }

        CmpInst cond = head.getBrCond();
        CmpInst cond2 = head2.getBrCond();
        if(cond == null || cond2 == null) return false;

        /*
        *   融合所需相同的条件
        *   1. itEnd相同
        *   2. 迭代次数相同
        *   3. store存储位置相同
        *   4. 不能有call指令
        * */

        if(!loop.getItInit().equals(loop2.getItInit())){
            return false;
        }
        if(!loop.getItEnd().equals(loop2.getItEnd())){
            return false;
        }
        if(!loop.getItStep().equals(loop2.getItStep())){
            return false;
        }

        //  确保循环内除了迭代alu外没有对i-1/i+1的引用
        int binCnt = 0;
        itVar = loop.getItVar();
        itVar2 = loop2.getItVar();
        for(User user : itVar.getUserList()){
            if(user instanceof BinaryInst binInst && !binInst.getOp().isCmpOP()){
                binCnt++;
            }
        }
        if(binCnt != 1){
            return false;
        }

        StoreInst storeInst = null;
        StoreInst storeInst2 = null;
        for(BasicBlock bb : loop.getBbs()){
            for(IList.INode<Instruction, BasicBlock> instNode : bb.getInsts()){
                Instruction inst = instNode.getValue();
                if(inst instanceof StoreInst _storeInst){
                    if(storeInst == null){
                        storeInst = _storeInst;
                    }
                    else return false;
                }
                else if(inst instanceof CallInst){
                    return false;
                }
            }
        }
        for(BasicBlock bb : loop2.getBbs()){
            for(IList.INode<Instruction, BasicBlock> instNode : bb.getInsts()){
                Instruction inst = instNode.getValue();
                if(inst instanceof StoreInst _storeInst){
                    if(storeInst2 == null){
                        storeInst2 = _storeInst;
                    }
                    else return false;
                }
                else if(inst instanceof CallInst){
                    return false;
                }
            }
        }

        if(storeInst == null || storeInst2 == null){
            return false;
        }
        if(!isEqual(storeInst.getPointer(), storeInst2.getPointer())){
            return false;
        }
        return true;
    }

    private boolean isEqual(Value val1, Value val2){
        if(val1 instanceof GepInst gepInst && val2 instanceof GepInst gepInst2){
            if(!gepInst.getTarget().equals(gepInst2.getTarget())){
                return false;
            }
            if(gepInst.getOperands().size() != gepInst2.getOperands().size()){
                return false;
            }
            for(int i = 0; i < gepInst.getOperands().size(); i++){
                if(!isEqual(gepInst.getOperand(i), gepInst2.getOperand(i))){
                    return false;
                }
            }
            return true;
        }
        else if(val1 instanceof LoadInst loadInst && val2 instanceof LoadInst loadInst2){
            return isEqual(loadInst.getPointer(), loadInst2.getPointer());
        }
        else if(val1.equals(val2)){
            return true;
        }
        else if(val1.equals(itVar) && val2.equals(itVar2)){
            return true;
        }
        else if(val2.equals(itVar) && val1.equals(itVar2)){
            return true;
        }
        return false;
    }

    @Override
    public String getName() {
        return "LoopFusion";
    }
}
