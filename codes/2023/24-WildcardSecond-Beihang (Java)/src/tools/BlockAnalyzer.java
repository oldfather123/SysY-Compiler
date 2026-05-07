package tools;

import ir.Value;
import ir.instr.Br;
import ir.instr.Instr;
import ir.value.BasicBlock;
import ir.value.Module;

public class BlockAnalyzer {
    //要求已经经过了处理，成为了一维扁平block表
    public static void updateBlockPredAndSuccs(Module module) {
        for(var func : module.functions.values()){
            for(var block : func.blocks){
                block.getPreds().clear();
                block.getSuccs().clear();
            }
            for(var block : func.blocks){
                if((block.getInsts().get(
                    block.getInsts().size() - 1
                )) instanceof Br){
                    for(var sub : ((Br) block.getInsts().get(block.getInsts().size()-1)).getOperands()){
                        if(sub instanceof BasicBlock){
                            ((BasicBlock) sub).addPreds(block);
                            block.addSuccs((BasicBlock) sub);
                        }
                    }
                }
            }
            for(var block : func.blocks){
                for(Value instr : block.getInsts()){
                    ((Instr)instr).setParent(block);
                }
            }
        }
    }
}
