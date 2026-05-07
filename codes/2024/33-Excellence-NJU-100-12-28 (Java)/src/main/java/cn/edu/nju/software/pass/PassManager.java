package cn.edu.nju.software.pass;

import cn.edu.nju.software.ir.basicblock.BasicBlockRef;
import cn.edu.nju.software.ir.module.ModuleRef;
import cn.edu.nju.software.ir.value.FunctionValue;

import java.util.ArrayList;
import java.util.List;

public class PassManager {
   private final ModuleRef module;
   List<Pass> allPasses;
    public PassManager(ModuleRef module) {
        this.module = module;
        allPasses=new ArrayList<>();
        register();
    }

    public boolean runPass() {
        boolean changed = false;
        for (Pass pass : allPasses) {
            if(pass instanceof ModulePass modulePass){
               changed|= modulePass.runOnModule(module);
            }else if(pass instanceof FunctionPass functionPass){
                for (FunctionValue functionValue: module.getFunctions()){
                    changed|= functionPass.runOnFunction(functionValue);
                }
            }else if(pass instanceof BasicBlockPass basicBlockPass){
                for (FunctionValue functionValue: module.getFunctions()){
                    for (BasicBlockRef basicBlockRef:functionValue.getBasicBlockRefs()){
                        changed|= basicBlockPass.runOnBasicBlock(basicBlockRef);
                    }
                }
            }
        }
        return changed;
    }

    //TODO:add pass here
    private void register(){
        allPasses.add(CFGBuildPass.getInstance());
        allPasses.add(LoopBuildPass.getInstance());
        allPasses.add(GlobalToLocalPass.getInstance());
        allPasses.add(GEPReductionPass.getInstance());
//        allPasses.add(new FunctionInlinePass());
//        allPasses.add(new LoopInvariantCodeMotionPass());
        allPasses.add(new RedundantBlockEliminationPass());
        allPasses.add(MemToReg.getInstance());
        allPasses.add(EliminateDeadCode.getInstance());
        allPasses.add(MergeRepeatedArithmeticPass.getInstance());
        allPasses.add(StrengthReductionPass.getInstance());
        allPasses.add(IdentifyTmpPass.getInstance());
        allPasses.add(EliminateConstExp.getInstance());
        allPasses.add(RegToMem.getInstance());
        allPasses.add(EliminateLoadStorePass.getInstance());
    }
    public void setDbgFlag(){
        for(Pass pass:allPasses){
            pass.setDbgFlag();
        }
    }

}
