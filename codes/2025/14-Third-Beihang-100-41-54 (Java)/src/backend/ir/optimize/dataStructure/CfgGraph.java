package backend.ir.optimize.dataStructure;

import backend.ir.entity.BasicBlock;
import backend.ir.entity.Function;
import backend.ir.entity.insts.BranchInst;
import backend.ir.entity.insts.Inst;
import backend.ir.entity.insts.UCJumpInst;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class CfgGraph {
    //流入的基本块，这里面存的就是每个基本块的前序基本块
    private Map<BasicBlock, List<BasicBlock>> inBlocks;
    //流出的基本块，这里面存的就是每个基本块的后序基本块
    private Map<BasicBlock, List<BasicBlock>> outBlocks;
    private Function function = null;

    public CfgGraph() {}

    public CfgGraph(Function function) {
        this.function = function;
        inBlocks = new HashMap<>();
        outBlocks = new HashMap<>();
    }

    public Map<BasicBlock, List<BasicBlock>> getInBlocks() {
        return inBlocks;
    }

    public Map<BasicBlock, List<BasicBlock>> getOutBlocks() {
        return outBlocks;
    }

    public Function getFunction() {
        return function;
    }

    /**
     * 构造这个函数的控制流图
     */
    public void buildCFG(){
        List<BasicBlock> functionBasicBlocks = function.getAllBasicBlocks();
        for(BasicBlock basicBlock : functionBasicBlocks){
            Inst lastInst = basicBlock.getLastInst();
            if(lastInst instanceof UCJumpInst ucjumpInst){
                //直接跳转指令
                addEdge(basicBlock,(BasicBlock) ucjumpInst.getUsees().get(0));
            }else if(lastInst instanceof BranchInst branchInst){
                //是条件跳转指令，流图要建立向if内的基本块和外面的基本块
                BasicBlock trueBlock = (BasicBlock) branchInst.getUsees().get(1);
                BasicBlock falseBlock = (BasicBlock) branchInst.getUsees().get(2);
                addEdge(basicBlock,trueBlock);
                addEdge(basicBlock,falseBlock);
            }
        }
    }

    /**
     * 在两个基本块之间添加边，分别加入前驱map和后驱map
     *
     * @param fromBlock 从哪个block来
     * @param toBlock   到哪个block去
     */
    public void addEdge(BasicBlock fromBlock, BasicBlock toBlock){
        addInBlock(toBlock, fromBlock);
        addOutBlock(fromBlock, toBlock);
    }

    /**
     * 向两个基本块中间插入一个基本块
     * 用于消除phi指令
     *
     * @param fromBlock 前序基本块
     * @param toBlock   后序基本块
     * @param midBlock  中间基本块
     */
    public void insertBlock(BasicBlock fromBlock, BasicBlock toBlock, BasicBlock midBlock){
        if(!outBlocks.containsKey(fromBlock) && !inBlocks.containsKey(toBlock)){
            return;
        }
        //to_block在outBlock的位置
        int toBlockOut = outBlocks.get(fromBlock).indexOf(toBlock);
        //from_block在inBlock的位置
        int fromBlockIn = inBlocks.get(toBlock).indexOf(fromBlock);
        //在对应位置添加中间块
        addInBlock(toBlock, midBlock, fromBlockIn);
        addOutBlock(fromBlock, midBlock, toBlockOut);
        //移除原本的基本块
        outBlocks.get(fromBlock).remove(toBlock);
        inBlocks.get(toBlock).remove(fromBlock);
        //将midBlock与两个基本块连接起来
        addOutBlock(fromBlock, midBlock);
        addInBlock(toBlock, midBlock);
    }

    /**
     * 将两个基本块进行合并，将后面的基本块合并在前面, 修改对应的in和out
     *
     * @param fromBlock 前序基本块
     * @param toBlock   后序基本块
     */
    public void fromMergeToBlock(BasicBlock fromBlock, BasicBlock toBlock){
        // 从 from_block 的 outBlocks 中移除 to_block
        List<BasicBlock> fromOutBlocks = fromBlock.getOutBlocks();
        fromOutBlocks.remove(toBlock);
        // 遍历 to_block 的 outBlocks
        List<BasicBlock> basicOutBlocks = toBlock.getOutBlocks();
        for(BasicBlock outBasicBlock : basicOutBlocks){
            // 添加到 from_block 的 outBlocks 中
            fromOutBlocks.add(outBasicBlock);
            // 从 outBasicBlock 的 inBlocks 中移除 from_block
            List<BasicBlock> outInBlocks = outBasicBlock.getInBlocks();
            outInBlocks.remove(toBlock);
            // 添加 from_block 到 outBasicBlock 的 inBlocks 中
            outInBlocks.add(fromBlock);
        }
    }

    /**
     * 将两个基本块进行合并，将前面的基本块合并在后面, 修改对应的in和out
     *
     * @param fromBlock 前序基本块
     * @param toBlock   后序基本块
     */
    public void toMergeFromBlock(BasicBlock fromBlock, BasicBlock toBlock){
        // 从 to_block 的 inBlocks 中移除 from_block
        List<BasicBlock> toInBlocks = toBlock.getInBlocks();
        toInBlocks.remove(fromBlock);
        // 遍历 from_block 的 inBlocks
        List<BasicBlock> basicInBlocks = fromBlock.getInBlocks();
        for(BasicBlock inBasicBlock : basicInBlocks){
            // 添加到 inBasicBlock 的 inBlocks 中
            if(!toInBlocks.contains(inBasicBlock))
                toInBlocks.add(inBasicBlock);
            // 从 inBasicBlock 的 inBlocks 中移除 from_block
            List<BasicBlock> inOutBlocks = inBasicBlock.getOutBlocks();
            inOutBlocks.remove(fromBlock);
            // 添加 to_block 到 inBasicBlock 的 outBlocks 中
            if(!inOutBlocks.contains(toBlock))
                inOutBlocks.add(toBlock);
        }
    }

    /**
     * 输出控制流图，debug用
     */
    public String debugCFG(){
        StringBuilder cfg= new StringBuilder();
        cfg.append("函数: ").append(function.getName()).append("\n\t\t");
        List<BasicBlock> basicBlocks = function.getAllBasicBlocks();
        for(BasicBlock basicBlock : basicBlocks){
            if(!basicBlock.isExist()) continue;
            cfg.append("基本块: ").append(basicBlock.getName()).append("\n\t\t\t");
            cfg.append("inBlocks: \n\t\t\t\t");
            List<BasicBlock> newInBlocks = basicBlock.getInBlocks();
            if(newInBlocks != null){
                for(BasicBlock inBlock : newInBlocks){
                    cfg.append(inBlock.getName()).append("\n\t\t\t\t");
                }
            }
            cfg.append("\n\t\t\t");
            cfg.append("outBlocks: \n\t\t\t\t");
            List<BasicBlock> newOutBlocks = basicBlock.getOutBlocks();
            if(newOutBlocks != null){
                for(BasicBlock outBlock : newOutBlocks){
                    cfg.append(outBlock.getName()).append("\n\t\t\t\t");
                }
            }
            cfg.append("\n\t\t");
        }
        return cfg.toString();
    }

    /**
     * 向流图添加前序基本块
     *
     * @param toBlock   到哪个block去
     * @param fromBlock 从哪个block来
     * @param index     将基本块from_block插入到to_block前序的哪个位置？一般用于在两个基本块之间插
     */
    private void addInBlock(BasicBlock toBlock,BasicBlock fromBlock, int... index){
        inBlocks.computeIfAbsent(toBlock, k -> new ArrayList<>());
        if(index.length == 1 && index[0] <= inBlocks.get(toBlock).size()) {
            //说明输入了参数，要在指定位置添加block
            // 获取迭代器指向插入位置
            List<BasicBlock> vec = inBlocks.get(toBlock);
            vec.add(index[0], fromBlock); // 在指定位置插入
        }else {
            //在最后添加
            List<BasicBlock> vec = inBlocks.get(toBlock);
            vec.add(fromBlock); // 在最后插入
        }
    }

    /**
     * 向流图添加后序基本块
     *
     * @param fromBlock 从哪个block来
     * @param toBlock   到哪个block去
     * @param index     将基本块插入到哪个位置？一般用于在两个基本块之间插
     */
    private void addOutBlock(BasicBlock fromBlock,BasicBlock toBlock, int... index){
        outBlocks.computeIfAbsent(fromBlock, k -> new ArrayList<>());
        if(index.length == 1 && index[0] <= outBlocks.get(fromBlock).size()) {
            //说明输入了参数，要在指定位置添加block
            // 获取迭代器指向插入位置
            List<BasicBlock> vec = outBlocks.get(fromBlock);
            vec.add(index[0], toBlock); // 在指定位置插入
        }else {
            //在最后添加
            List<BasicBlock> vec = outBlocks.get(fromBlock);
            vec.add(toBlock); // 在最后插入
        }
    }
}
