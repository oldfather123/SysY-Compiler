package backend.ir.optimize.tool;

import backend.ir.entity.BasicBlock;
import backend.ir.entity.Function;
import backend.ir.entity.Value;
import backend.ir.entity.insts.BranchInst;
import backend.ir.entity.insts.Inst;
import backend.ir.entity.insts.RetInst;
import backend.ir.entity.insts.UCJumpInst;

import java.util.*;

/**
 * 删除死代码
 * 主要原因是有一些return，跳转指令后的语句，这些没有必要保留
 */
public class DCETool {

    private static final Map<BasicBlock, Boolean> visitBlock = new HashMap<>();

    /**
     * 删除基本块内的分支后代码。
     * 对于一个基本块，跳转/return代码之后的所有代码都不会到达，直接删除即可
     * 而且如果不删除，那么在插入phi函数会出现异常
     * @param basicBlock 基本块
     */
    public static void deleteBranchInstruction(BasicBlock basicBlock){
        List<Value> usees = basicBlock.getUsees();
        int i = 0;
        for(i=0;i<usees.size();i++){
            Value inst = usees.get(i);
            if(inst instanceof BranchInst
                    || inst instanceof UCJumpInst
                    || inst instanceof RetInst){
                //记录i，从i+1的位置的指令都要删除
                break;
            }
        }
        for(int j = i + 1;j < usees.size();j++){
            Value inst = usees.get(j);
            if(inst instanceof Inst){
                basicBlock.removeUsee(inst);
            }
        }
    }

    /**
     * 删除函数中不可到达的基本块
     * 主要原因是，有些块不可到达，但是后面流图可能会有很多后继，导致构建支配集的时候出现问题
     * 进而影响支配树的生成
     * 采用dfs暴力搜索
     * @param function 函数
     */
    public static void deleteBlock(Function function){
        //获取第一个基本块，这个就是流图的入口。所有不可达的基本块都应当剔除
        BasicBlock firstBlock = function.getSuperBlock();
        List<BasicBlock> basicBlocks = function.getAllBasicBlocks();
        visitBlock.clear();
        dfsDeleteBlock(firstBlock);
        Iterator<BasicBlock> it = basicBlocks.iterator();
        while (it.hasNext()) {
            BasicBlock block = it.next();
            if (!visitBlock.containsKey(block) || !visitBlock.get(block)) {
                // This block wasn't visited
                block.setExist(false);
                it.remove();
            }
        }
    }

    private static void dfsDeleteBlock(BasicBlock startBlock) {
        Stack<BasicBlock> stack = new Stack<>();
        stack.push(startBlock);
        visitBlock.put(startBlock, true); // 标记起始块为已访问

        while (!stack.isEmpty()) {
            BasicBlock currentBlock = stack.pop();
            Inst inst = currentBlock.getLastInst();

            if (inst instanceof UCJumpInst) {
                // 处理无条件跳转（只有一个后继块）
                BasicBlock desBlock = (BasicBlock) inst.getUsee(0);
                if (desBlock != null && !visitBlock.containsKey(desBlock)) {
                    visitBlock.put(desBlock, true); // 标记为已访问
                    stack.push(desBlock);           // 压栈继续处理
                }
            } else if (inst instanceof BranchInst) {
                // 处理条件跳转（有两个后继块）
                BasicBlock trueBlock = (BasicBlock) inst.getUsee(1);
                BasicBlock falseBlock = (BasicBlock) inst.getUsee(2);

                // 注意：栈是后进先出，为了保证顺序，先压 falseBlock，再压 trueBlock
                if (falseBlock != null && !visitBlock.containsKey(falseBlock)) {
                    visitBlock.put(falseBlock, true);
                    stack.push(falseBlock);
                }
                if (trueBlock != null && !visitBlock.containsKey(trueBlock)) {
                    visitBlock.put(trueBlock, true);
                    stack.push(trueBlock);
                }
            }
        }
    }
}
