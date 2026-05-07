package mid.Optimizer.RedundancyElim;

import mid.IntermediatePresentation.BasicBlock;
import mid.IntermediatePresentation.Function.Function;
import mid.IntermediatePresentation.IRManager;
import mid.IntermediatePresentation.Instruction.ALU;
import mid.IntermediatePresentation.Instruction.Br;
import mid.IntermediatePresentation.Instruction.Cmp;
import mid.IntermediatePresentation.Instruction.Instruction;
import mid.IntermediatePresentation.Value;
import mid.Optimizer.ControllFlow.ControlFlowGraph;
import mid.Optimizer.Optimizer;
import mid.Optimizer.Utils.Range;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;

public class JumpThreading {
    /*
    每个block都有一个变量取值范围表，>,<,>=,<=,!=
    遇到if更新后继变量范围，并查当前范围是否一定满足/不满足if
    // todo:现在没有考虑到每条分支的信息单独存，得测试一下这种情况多不多）比如某块的两个前驱更新了同一个变量，应该用符号表     （问题：现在的变量范围传播会使得先更新的分支信息被覆盖）
    // 要支持循环的话也是上述情况
    */

    public void optimize() {
        ControlFlowGraph cfg = Optimizer.instance().getCFG();
        for (Function function : IRManager.getModule().getDecledFunctions()) {
            ArrayList<BasicBlock> basicBlocks = Optimizer.instance().bfsDominTreeArray(function.getEntranceBlock());
            for (BasicBlock bb : basicBlocks) {
                if (bb.getLoopDepth() != 0) continue;
                for (Instruction inst : bb.getInstructionList()) {
                    if (inst instanceof ALU alu) {
                        bb.updateVarRange(alu);
                    }
                }
//                bb.printRange();

                // todo（问题：直接向child复制有可能出了变量的存活范围；用符号表好像也不行，因为一个变量在不同位置有不同的取值）
                // 常量(取值范围)传播
                HashSet<BasicBlock> children = cfg.getChildren(bb);
                if (children != null)
                    for (BasicBlock child : children) {
                        child.copyRanges(bb);
                    }
                if (!(bb.getLastInstruction() instanceof Br br && br.getOperandList().size() == 3)) continue;
                Cmp cmp = (Cmp) br.getCond(); // x>3
                Value var = cmp.getOperandList().get(0);
                BasicBlock ifTrue = br.getIfTrue();
                BasicBlock ifFalse = br.getIfFalse();
                ifTrue.updateVarRange(cmp, true);
                // ifFalse.updateVarRange(cmp, false);      //必须有else才能更新false，如果不是，range不能确定
                HashSet<BasicBlock> parents = cfg.getParents(bb);
                if (parents == null) continue;

                // todo：同上个todo，应该递归查，即先在bb查，再在parent查；如果在bb查到是parent共用一个新的block；如果在parent查到则给查到的parent后面接一个新的block，没查到的parent不动
                Range range = bb.getVarRange(var);

                // 重新构造block
                int withinRange = range.isWithinRange(cmp);
                if (withinRange != 0) {
                    BasicBlock block = new BasicBlock();
                    for (Instruction inst : bb.getInstructionList()) {
                        if (inst instanceof Br) continue;
                        // 重新构造inst
//                        Instruction newInst = new Instruction(IRManager.getInstance().declareTempVar(), inst.getType());
//                        newInst.setOperandList(inst.getOperandList());
//                        block.addInstruction(newInst);
//                        inst.beReplacedBy(newInst);
                        block.addInstruction(inst);
                    }
                    Iterator<Instruction> iterator = bb.getInstructionList().iterator();
                    while (iterator.hasNext()) {
                        Instruction inst = iterator.next();
                        if (inst instanceof Br) continue;
                        iterator.remove();
                    }
                    // todo: 做的有些麻烦（）正常是重新创建inst，但是没法对应到具体的inst子类，所以就在新的block里添加原来bb的inst，把bb的inst移除
                    if (withinRange == 1)
                        block.addInstruction(new Br(ifTrue));
                    else if (withinRange == 2)
                        block.addInstruction(new Br(ifFalse));
                    bb.getFunction().addBlock(block);
                    for (BasicBlock parent : parents) {
                        Br br1 = (Br) parent.getLastInstruction();
                        br1.replaceOperand(bb, block);
                    }
                    if (bb.getUserList().isEmpty()) {
                        bb.getFunction().removeBlock(bb);
                        bb.destroy();           //正规做时删掉这步，因为现在是parent共用一个新的block所以原来的不需要了
                    }
                }
            }
        }

//        Module module = IRManager.getModule();
//        for (Function f : module.getDecledFunctions()) {
//            for (BasicBlock b : f.getBlocks()) {
//                System.out.print(b.getName() + ": ");
//                b.printRange();
//            }
//        }
    }
}
