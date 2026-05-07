package Pass.IR;

import IR.IRModule;
import IR.Value.*;
import IR.Value.Instructions.*;
import Pass.Pass;

public class MathOptimization implements Pass.IRPass {
    @Override
    public void run(IRModule module) {
        for(Function function : module.getFunctions()){
            if(canMulOptimize(function)){
                mulOptimize(function);
            }
        }
    }

    private Value modValue;

    private void mulOptimize(Function function){

    }

    private boolean canMulOptimize(Function function){
        BasicBlock bbEntry = function.getBbEntry();
        if(function.getArgs().size() != 2){
            return false;
        }
        Argument a = function.getArgs().get(0);
        Argument b = function.getArgs().get(1);

        if(!(bbEntry.getFirstInst() instanceof BinaryInst binInst)){
            return false;
        }
        if(!binInst.getLeftVal().equals(b) && !(binInst.getRightVal() instanceof ConstInteger constInt && constInt.getValue() == 0)){
            return false;
        }

        BrInst brInst = (BrInst) bbEntry.getLastInst();
        if(brInst.isJump()) return false;
        BasicBlock trueBb = brInst.getTrueBlock();
        BasicBlock falseBb = brInst.getFalseBlock();

        if(trueBb.getInsts().getSize() != 1 || !(trueBb.getLastInst() instanceof RetInst retInst)){
            return false;
        }

        if(!(falseBb.getFirstInst() instanceof BinaryInst binInst2)){
            return false;
        }
        if(!binInst2.getLeftVal().equals(b) && !(binInst.getRightVal() instanceof ConstInteger constInt && constInt.getValue() == 1)){
            return false;
        }
        BrInst brInst2 = (BrInst) falseBb.getLastInst();
        if(brInst2.isJump()) return false;
        trueBb = brInst.getTrueBlock();
        falseBb = brInst.getFalseBlock();
        return true;
    }

    @Override
    public String getName() {
        return "MathOptimization";
    }
}
