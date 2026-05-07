package frontend.ir.cloner;

import frontend.ir.Value;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.instr.terminator.ReturnInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;

import java.util.HashSet;
import java.util.List;

public class FunctionCloner extends Cloner {
    private static FunctionCloner instance = null;

    private static final HashSet<ReturnInstr> retSet = new HashSet<>();
    private static BasicBlock entryPoint = null;

    private FunctionCloner() {
        super();
    }

    public static FunctionCloner getInstance() {
        if (instance == null) {
            instance = new FunctionCloner();
        }
        return instance;
    }

    @Override
    public void cloneInstr(Instruction instruction, BasicBlock targetBB, Function targetFunc) {
        if (instruction instanceof ReturnInstr ret) {
            Value retVal = newValueGetter(ret.getReturnValue());
            ReturnInstr newRet = new ReturnInstr(retVal, targetBB);
            retSet.add(newRet);
        } else {
            super.cloneInstr(instruction, targetBB, targetFunc);
        }
    }

    public void makeBBgraph(Function targetFunc, Function sourceFunc, BasicBlock preBB) {
        BasicBlock insertPoint = preBB;
        for (BasicBlock srcBB : sourceFunc.getBasicBlockList()) {
            BasicBlock newBB = new BasicBlock(targetFunc.getAndAddBlkIdx());
            if (entryPoint == null) {
                entryPoint = newBB;
                preBB.open();
                new JumpInstr(newBB, preBB);
            }
            newBB.insertAfter(insertPoint);
            insertPoint = newBB;
            valueProjection.put(srcBB, newBB);
        }
    }

    public void cloneFParm(Function sourceFunc, List<Value> rParms) {
        for (int i = 0; i < sourceFunc.getFParams().size(); i++) {
            valueProjection.put(sourceFunc.getFParams().get(i), rParms.get(i));
        }
    }

    public void removeRet(BasicBlock aftBB, CallInstr callInstr) {
        if (callInstr.isVoidCall()) {
            callInstr.removeFromList();
            for (ReturnInstr ret : retSet) {
                ret.forceRemoveFromList();
                new JumpInstr(aftBB, ret.getParentBB());
            }
        } else {
            PhiInstr phi = new PhiInstr(callInstr.getType(), callInstr.getRegIndex(), aftBB);
            callInstr.replaceUseWith(phi);
            aftBB.insertInsToHead(phi);
            callInstr.removeFromList();
            for (ReturnInstr ret : retSet) {
                phi.addOperand(ret.getParentBB(), ret.getReturnValue());
                ret.forceRemoveFromList();
                new JumpInstr(aftBB, ret.getParentBB());
            }
            phi.simplify();
        }
    }

    public void cloneFunction(Function sourceFunc, Function targetFunc, BasicBlock preBB, List<Value> rValue) {
        valueProjection.clear();
        needPhi.clear();
        retSet.clear();
        entryPoint = null;
        cloneFParm(sourceFunc, rValue);
        makeBBgraph(targetFunc, sourceFunc, preBB);
        cloneBlockDfs(sourceFunc.getFirstBlk(), (BasicBlock) valueProjection.get(sourceFunc.getFirstBlk()), targetFunc);
        makePhi(targetFunc);
    }
}
