package midend.optimism;

import midend.BasicBlock;
import midend.Function;
import midend.Module;
import midend.analysis.Loop;
import midend.analysis.LoopInfo;
import midend.instr.BrInstr;
import midend.instr.Instruction;

public class RemoveLoop {
    private Module module;

    public RemoveLoop(Module module) {
        this.module = module;
    }

    public void run() {
        for (Function function : module.getFunctions()) {
            for (Loop loop : function.getLoops()) {
                LoopInfo.analysisUseLess(loop);
                if (loop.isUseLess()) {
                    BasicBlock block = (loop.getBasicBlocks().contains(loop.getHeader().getPreBlock().get(0)))? loop.getHeader().getPreBlock().get(1) : loop.getHeader().getPreBlock().get(0);
                    BasicBlock exit = loop.getExitBlock().get(0);
                    ((BrInstr) block.getInstrList().getLast()).modifyBlock(loop.getHeader(), exit);
                    for (BasicBlock basicBlock : loop.getBasicBlocks()) {
                        for (Instruction instruction : basicBlock.getInstrList()) {
                            instruction.remove();
                        }
                    }
                    function.getBlockList().removeAll(loop.getBasicBlocks());
                }
            }
        }
    }
}
