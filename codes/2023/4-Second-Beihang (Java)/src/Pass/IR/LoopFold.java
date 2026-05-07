package Pass.IR;

import IR.IRBuildFactory;
import IR.IRModule;
import IR.Type.IntegerType;
import IR.Value.BasicBlock;
import IR.Value.ConstInteger;
import IR.Value.Function;
import IR.Value.Instructions.*;
import IR.Value.Value;
import Pass.IR.Utils.IRLoop;
import Pass.IR.Utils.LoopAnalysis;
import Pass.Pass;
import Utils.DataStruct.IList;

import java.util.ArrayList;
import java.util.HashSet;

/*
* 优化关于 while(i < n){ ans = ans + base; i = i + 1; }
* */
public class LoopFold implements Pass.IRPass {
    private Value loopCount;
    private Value addValue;
    private Value modValue;
    private Value ansValue;
    private final IRBuildFactory f = IRBuildFactory.getInstance();

    @Override
    public void run(IRModule module) {
        for(Function function : module.getFunctions()){
            LoopAnalysis.runLoopInfo(function);
            LoopAnalysis.runLoopIndvarInfo(function);
            for(IRLoop loop : function.getTopLoops()){
                runLoopFold(loop);
            }
        }
    }

    private void runLoopFold(IRLoop loop){
        for(IRLoop subLoop : loop.getSubLoops()){
            runLoopFold(subLoop);
        }

        if(canFold(loop)){
            BasicBlock head = loop.getHead();
            Value loopCount = loop.getItEnd();
            BasicBlock entering = loop.getHead().getPreBlocks().get(0);
            BrInst brInst = (BrInst) entering.getLastInst();
            BinaryInst modInst = f.getBinaryInst(loopCount, modValue, OP.Mod, IntegerType.I32);
            modInst.insertBefore(brInst);
            modInst.setI64();
            BinaryInst mulInst = f.getBinaryInst(modInst, addValue, OP.Mul, IntegerType.I32);
            mulInst.insertBefore(brInst);
            mulInst.setI64();
            BinaryInst modInst2 = f.getBinaryInst(mulInst, modValue, OP.Mod, IntegerType.I32);
            modInst2.insertBefore(brInst);
            modInst2.setI64();
            ansValue.replaceUsedWith(modInst2);

            BasicBlock exitBb = null;
            for(BasicBlock exit : loop.getExitBlocks()){
                exitBb = exit;
            }
            brInst.setJumpBlock(exitBb);
            entering.removeNxtBlock(head);
            entering.setNxtBlock(exitBb);
            assert exitBb != null;
            int idx = exitBb.getPreBlocks().indexOf(head);
            exitBb.getPreBlocks().set(idx, entering);

            for(BasicBlock bb : loop.getBbs()){
                for(IList.INode<Instruction, BasicBlock> instNode : bb.getInsts()){
                    Instruction inst = instNode.getValue();
                    inst.removeUseFromOperands();
                }
                bb.getNode().removeFromList();
            }
        }
    }

    private boolean canFold(IRLoop loop){
        BasicBlock exit = null;

        if(!loop.isSimpleLoop() || !loop.isSetIndVar()){
            return false;
        }
        if(!(loop.getItStep() instanceof ConstInteger con
                && con.getValue() == 1)){
            return false;
        }
        if(!(loop.getItInit() instanceof ConstInteger _con
                && _con.getValue() == 0)){
            return false;
        }
        if(loop.getExitBlocks().size() != 1){
            return false;
        }
        for(BasicBlock exitBlock : loop.getExitBlocks()){
            exit = exitBlock;
        }
        assert exit != null;
        //  Latch
        BasicBlock latch = loop.getLatchBlocks().get(0);
        if(latch.getInsts().getSize() != 4){
            return false;
        }

        HashSet<Value> allIndInsts = new HashSet<>();
        allIndInsts.add(loop.getItVar());     //  i
        allIndInsts.add(loop.getHeadBrCond());
        allIndInsts.add(loop.getItAlu());      //  i = i + 1
        allIndInsts.add(loop.getHead().getLastInst());
        allIndInsts.add(latch.getLastInst());

        Phi ansInst = null;
        BinaryInst addInst = null;
        BinaryInst modInst = null;
        ArrayList<Instruction> notIndInsts = new ArrayList<>();
        for(BasicBlock bb : loop.getBbs()){
            for(IList.INode<Instruction, BasicBlock> instNode : bb.getInsts()){
                Instruction inst = instNode.getValue();
                if(!allIndInsts.contains(inst)){
                    notIndInsts.add(inst);
                    if(inst instanceof Phi phi){
                        ansInst = phi;
                    }
                    else if(inst instanceof BinaryInst binInst){
                        if(binInst.getOp().equals(OP.Add)){
                            addInst = binInst;
                        }
                        else if(binInst.getOp().equals(OP.Mod)){
                            modInst = binInst;
                        }
                    }
                }
            }
        }

        if(notIndInsts.size() != 3){
            return false;
        }
        if(ansInst == null || addInst == null || modInst == null){
            return false;
        }

        //  ans = ans + const
        if(!(ansInst.getOperand(0) instanceof ConstInteger constInt
                && constInt.getValue() == 0
                && ansInst.getOperand(1).equals(modInst))){
            return false;
        }
        if(!(addInst.getLeftVal().equals(ansInst)
                && addInst.getRightVal() instanceof ConstInteger constInt2)){
            return false;
        }
        if(!(modInst.getLeftVal().equals(addInst)
                && modInst.getRightVal() instanceof ConstInteger constInt3)){
            return false;
        }


        ansValue = ansInst;
        addValue = constInt2;
        modValue = constInt3;
        return true;
    }

    @Override
    public String getName() {
        return "LoopFold";
    }
}
