package tools;

import ir.Value;
import ir.instr.Instr;
import ir.value.BasicBlock;
import ir.value.Module;
import ir.value.user.Function;

import java.util.ArrayList;

public class ModuleFlattener {
    static Module module;
    static int blockNo;
    static ArrayList<BasicBlock> newBlocks;
    static BasicBlock newblock;
    public static void flattenModule(Module module) {
        ModuleFlattener.module = module;
        for (Function func : module.functions.values()) {
            //----------------------------------------------------------------拍扁到一维
            newBlocks = new ArrayList<>();
            for (var block : func.blocks) {
                newblock = block;
                flattenBasicBlock(block);
                newBlocks.add(newblock);
            }
            func.blocks = newBlocks;
            //----------------------------------------------------------------对无end指令添加br
            for(int i = 0;i<newBlocks.size();){
                if(newBlocks.get(i).getInsts().isEmpty()||
                    !((Instr)newBlocks.get(i).getInsts().get(newBlocks.get(i).getInsts().size()-1))
                        .isEnd()){
                    newBlocks.get(i).getInsts().add(
                        IRBuilder.buildBr(null, newBlocks.get(i+1), null, null)
                    );
                }
                i++;
            }
            //----------------------------------------------------------------添加前驱
            IrRegDispatcher dispatcher = new IrRegDispatcher(func.getArgs().size()-1);
            for(var block : func.blocks) {
                block.name = dispatcher.allocId();
                for(var ir : block.getInsts()){
                    if(ir.hasName())
                        ir.name = dispatcher.allocId();
                }
            }
        }
    }


    public static void flattenBasicBlock(BasicBlock block){
        ArrayList<Value> insts = new ArrayList<>(block.getInsts());
        block.getInsts().clear();
        for(int i=  0;i < insts.size();i++){
            Value v = insts.get(i);
            if(v instanceof BasicBlock){
                newBlocks.add(newblock);
                newblock = (BasicBlock) v;
                flattenBasicBlock((BasicBlock) v);
            }else{
                newblock.getInsts().add(v);
            }
        }
    }
}
