package Pass.IR;

import IR.IRBuildFactory;
import IR.IRModule;
import IR.Type.IntegerType;
import IR.Value.*;
import IR.Value.Instructions.*;
import Pass.IR.Utils.UtilFunc;
import Pass.Pass;
import Utils.DataStruct.IList;

public class RecursiveUnRoll implements Pass.IRPass {

    private final IRBuildFactory f = IRBuildFactory.getInstance();

    @Override
    public void run(IRModule module) {
        UtilFunc.buildCallRelation(module);
        for(Function function : module.getFunctions()){
            if(function.getCalleeList().contains(function)){
                tryUnrollFunc(function);
            }
        }
    }

    private void tryUnrollFunc(Function function){
        if(function.getBbs().getSize() != 4){
            return;
        }
        if(function.getArgs().size() != 2){
            return;
        }
        Argument retVal = function.getArgs().get(0);
        Argument recNum = function.getArgs().get(1);
        BasicBlock head = function.getBbEntry().getNxtBlocks().get(0);
        BrInst brInst = (BrInst) head.getLastInst();
        if(brInst.isJump()){
            return;
        }
        if(!(head.getFirstInst() instanceof CmpInst cmpInst)){
            return;
        }
        if(!(cmpInst.getLeftVal().equals(recNum) && cmpInst.getRightVal() instanceof ConstInteger constz
                && constz.getValue() == 0 && cmpInst.getOp().equals(OP.Lt))){
            return;
        }


        BasicBlock latch = brInst.getTrueBlock();
        BasicBlock main = brInst.getFalseBlock();
        if(latch.getInsts().getSize() != 2){
            return;
        }
        RetInst retInst = (RetInst) latch.getLastInst();
        Value terminalValue = retInst.getValue();

        BinaryInst alu1 = null;
        BinaryInst alu2 = null;
        BinaryInst itAlu = null;
        RetInst mainRet = null;
        for(IList.INode<Instruction, BasicBlock> instNode : main.getInsts()){
            Instruction inst = instNode.getValue();
            if(inst instanceof BinaryInst binInst){
                if(binInst.getOp().equals(OP.Fadd)) {
                    alu1 = binInst;
                }
                else if(binInst.getOp().equals(OP.Fsub)){
                    alu2 = binInst;
                }
                else if(binInst.getOp().equals(OP.Sub)){
                    itAlu = binInst;
                }
            }
            else if(inst instanceof RetInst retInst1){
                mainRet = retInst1;
            }
        }

        assert alu1 != null;
        if(!(alu1.getLeftVal().equals(retVal)
                && alu1.getRightVal() instanceof CallInst callInst1
                && callInst1.getFunction().equals(function)
                && callInst1.getUseValues().get(0).equals(retVal)
                && callInst1.getUseValues().get(1).equals(itAlu))){
            return;
        }
        assert alu2 != null;
        if(!(alu2.getLeftVal().equals(alu1)
                && alu1.getRightVal() instanceof CallInst callInst2
                && callInst2.getFunction().equals(function)
                && callInst2.getUseValues().get(0).equals(alu1)
                && callInst2.getUseValues().get(1).equals(itAlu))){
            return;
        }
        if(!mainRet.getValue().equals(alu2)){
            return;
        }

        BinaryInst modInst = f.getBinaryInst(recNum, f.buildNumber(2), OP.Mod, IntegerType.I32);
        BinaryInst modCmpInst = f.getBinaryInst(modInst, f.buildNumber(0), OP.Ne, IntegerType.I1);
        cmpInst.replaceUsedWith(modCmpInst);
        modInst.insertBefore(modCmpInst);

    }

    @Override
    public String getName() {
        return "recursiveUnRoll";
    }
}
