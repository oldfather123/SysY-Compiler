package midpass;

import midend.value.BasicBlock;
import midend.value.Function;
import midend.value.instrs.Instr;
import midend.value.instrs.Jump;

import java.util.ArrayList;

public class MergeBB {
    private ArrayList<Function> functions;

    public MergeBB(ArrayList<Function> functions) {
        this.functions = functions;
        for (Function function : functions) {
            funcMergeBlock(function);
        }
    }

    //合并只有一条跳转语句的基本块
    public void funcMergeBlock(Function function) {
        BasicBlock entry = function.getFirstBlock();
        ArrayList<BasicBlock> removes = new ArrayList<>();
        for (var node : function.getBasicBlocks()) {
            BasicBlock block = node.get();
            if (block.equals(entry)) {
                continue;
            }
            int cnt = 0;
            for (var instrNode : block.getInstrList()) {
                cnt++;
            }
            if (cnt == 1) {
                if (block.getInstrList().firstElement() instanceof Jump) {
                    removes.add(block);
                }
            }
        }
        for (BasicBlock block : removes) {
            Instr instr = block.getLastInstr();
            BasicBlock tar = (BasicBlock) instr.getOperandList().get(0);
            ArrayList<BasicBlock> prevBlocks = new ArrayList<>(block.getPrevBB());
            for (BasicBlock prevBlock : prevBlocks) {
                prevBlock.changeBlockAtoB(block, tar);
                prevBlock.getSucBB().remove(block);
                prevBlock.getSucBB().add(tar);
                tar.getPrevBB().add(prevBlock);
            }
            tar.getPrevBB().remove(block);
            block.remove();
        }
    }
}
