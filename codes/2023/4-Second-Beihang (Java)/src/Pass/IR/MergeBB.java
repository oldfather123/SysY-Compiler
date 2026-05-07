package Pass.IR;

import IR.IRModule;
import IR.Value.*;
import IR.Value.Instructions.BrInst;
import IR.Value.Instructions.Instruction;
import IR.Value.Instructions.Phi;
import Pass.IR.Utils.UtilFunc;
import Pass.Pass;
import Utils.DataStruct.IList;

import java.util.ArrayList;

import static Pass.IR.Utils.UtilFunc.*;

/*
*   MergeBB将只有一条跳转指令的基本块删除
*   并修改与之相关的pre,nxt信息
*
* */
public class MergeBB implements Pass.IRPass {

    @Override
    public String getName() {
        return "MergeBB";
    }

    @Override
    public void run(IRModule module) {
        mergeSimpleBb(module);
        mergeBb(module);
    }

    private void mergeBb(IRModule module){
        for(Function function : module.getFunctions()){
            boolean change = true;
            while (change) {
                change = false;
                for (IList.INode<BasicBlock, Function> bbNode : function.getBbs()) {
                    BasicBlock bb = bbNode.getValue();
                    if (!(bb.getLastInst() instanceof BrInst brInst)) {
                        continue;
                    }
                    if (!brInst.isJump()) {
                        continue;
                    }

                    BasicBlock nxtBlock = brInst.getJumpBlock();
                    if (nxtBlock.getPreBlocks().size() != 1) {
                        continue;
                    }
                    if (nxtBlock.getFirstInst() instanceof Phi) {
                        continue;
                    }

                    ArrayList<Instruction> nxtInsts = new ArrayList<>();
                    for (IList.INode<Instruction, BasicBlock> nxtInstNode : nxtBlock.getInsts()) {
                        nxtInsts.add(nxtInstNode.getValue());
                    }

                    for (Instruction nxtInst : nxtInsts) {
                        nxtInst.removeFromBb();
                        nxtInst.insertAfter(bb.getLastInst());
                    }

                    brInst.removeSelf();

                    bb.removeNxtBlock(nxtBlock);
                    nxtBlock.removePreBlock(bb);

                    for(BasicBlock nxtNxtBlock : nxtBlock.getNxtBlocks()){
                        int idx = nxtNxtBlock.getPreBlocks().indexOf(nxtBlock);
                        nxtNxtBlock.getPreBlocks().set(idx, bb);
                        bb.setNxtBlock(nxtNxtBlock);
                    }

                    nxtBlock.getNode().removeFromList();

                    change = true;
                }
            }
            makeCFG(function);
        }
    }


    private void mergeSimpleBb(IRModule module){
        boolean change = true;
        for (Function function : module.getFunctions()) {
            while (change) {
                change = false;

                ArrayList<BasicBlock> deleteBbs = new ArrayList<>();
                for (IList.INode<BasicBlock, Function> bbNode : function.getBbs()) {
                    BasicBlock bb = bbNode.getValue();
                    //  entry特殊判断
                    if (bb.equals(function.getBbEntry())) {
                        continue;
                    }
                    if(canMerge(bb)){
                        change = true;
                        deleteBbs.add(bb);
                    }
                }

                for (BasicBlock deleteBb : deleteBbs) {
                    BasicBlock targetBb = getTargetBb((BrInst) deleteBb.getLastInst());
                    for (BasicBlock preBb : deleteBb.getPreBlocks()) {
                        preBb.turnBrBlock(deleteBb, targetBb);

                        preBb.removeNxtBlock(deleteBb);
                        preBb.setNxtBlock(targetBb);
                        targetBb.setPreBlock(preBb);
                    }
                    deleteBb.removeSelf();
                }

                //  entry特殊判断
                BasicBlock entry = function.getBbEntry();
                if(canMerge(entry)) {
                    BasicBlock newEntry = getTargetBb((BrInst) entry.getLastInst());
                    if(newEntry.getPreBlocks().size() == 0) {
                        entry.removeSelf();
                        IList.INode<BasicBlock, Function> newEntryNode = newEntry.getNode();
                        newEntryNode.removeFromList();
                        function.getBbs().addToHead(newEntryNode);
                    }
                }
            }
            makeCFG(function);
        }
    }

    private boolean canMerge(BasicBlock bb){
        if (involvePhi(bb)) {
            return false;
        }

        Instruction firstInst = bb.getFirstInst();
        Instruction lastInst = bb.getLastInst();
        if (firstInst.equals(lastInst)
                && lastInst instanceof BrInst brInst) {
            if(brInst.isJump()) {
                return true;
            }
            else{
                Value value = brInst.getJudVal();
                return value instanceof ConstInteger;
            }
        }
        return false;
    }

    private BasicBlock getTargetBb(BrInst brInst){
        BasicBlock targetBb;
        if(brInst.isJump()){
            targetBb = brInst.getJumpBlock();
        }
        else{
            ConstInteger constInt = (ConstInteger) brInst.getJudVal();
            if(constInt.getValue() == 1){
                targetBb = brInst.getTrueBlock();
            }
            else{
                targetBb = brInst.getFalseBlock();
            }
        }
        return targetBb;
    }
}
