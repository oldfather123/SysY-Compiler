package backend.ir.optimize.dataStructure;

import backend.ir.entity.BasicBlock;
import backend.ir.entity.Function;

import java.util.*;
import java.util.stream.Collectors;

/**
 * 求解支配树
 * 每个函数都有对应的支配树
 * 这里也包括求解支配边界
 */
public class DominantTree {
    private final Function function;

    /**
     * 支配集合，求这个基本块支配了哪些基本块。
     * 因为基本块在函数中是按照先后顺序存放的，所以这里按照hash存即可
     */
    Map<BasicBlock, List<BasicBlock>> dominateBlocks = new HashMap<>();
    /**
     * 支配边
     */
    Map<BasicBlock, List<BasicBlock>> dominateEdge = new HashMap<>();
    /**
     * 支配树，本质上支配树是求直接支配：
     * 若x严格支配y，并且不存在被x严格支配同时严格支配y的结点
     * 父节点 parentBlock, 子节点childBlock
     */
    Map<BasicBlock, BasicBlock> parentBlock = new HashMap<>();
    Map<BasicBlock, List<BasicBlock>> childBlocks = new HashMap<>();

    public Map<BasicBlock, List<BasicBlock>> getChildBlocks() {
        return childBlocks;
    }

    public Map<BasicBlock, BasicBlock> getParentBlock() {
        return parentBlock;
    }

    public Map<BasicBlock, List<BasicBlock>> getDominateBlocks() {
        return dominateBlocks;
    }

    public Map<BasicBlock, List<BasicBlock>> getDominateEdge() {
        return dominateEdge;
    }

    public DominantTree(Function function){
        this.function = function;
    }

    /**
     * 初始化支配集，每个基本块都把自己放进去
     */
    public void initDominateBlocks(){
        List<BasicBlock> basicBlocks = function.getAllBasicBlocks();
        for(BasicBlock basicBlock : basicBlocks){
            dominateBlocks.put(basicBlock, new ArrayList<>(Collections.singletonList(basicBlock)));
        }
    }

    /**
     * 生成该函数的支配集
     * 在for循环中找到所有不被 basicBlock支配的基本块，放入集合中
     * 这样所有不在集合中的基本块，都是被basicBlock支配的基本块
     * 这样可以将这些基本块加入到支配集里
     * 注意，这里生成的是每个节点支配了谁，而不是被支配；
     */
    public void generateDominateBlocks(){
        List<BasicBlock> basicBlocks = function.getAllBasicBlocks();
        BasicBlock entryBlock = basicBlocks.get(0);
        for(BasicBlock basicBlock : basicBlocks){
            Set<BasicBlock> noDominateBlocks = new HashSet<>();
            dfsDominateBlocks(entryBlock, basicBlock, noDominateBlocks);
            List<BasicBlock> dominateBlocks = new ArrayList<>();
            //将函数的所有基本块，取出必须经过basicBlock的基本块
            for(BasicBlock basicBlock1 : basicBlocks){
                if(!noDominateBlocks.contains(basicBlock1)){
                    dominateBlocks.add(basicBlock1);
                }
            }
            this.dominateBlocks.put(basicBlock, dominateBlocks);
        }
    }

    /**
     * 利用dfs进行深搜，找到所有不被block支配的基本块，放入集合中
     * 原理：
     * 当我们的入口基本块和所求的目标基本块相等时，函数停止
     * 这意味着集合并不会加入目标基本块block的后继
     * 如果block的某个后继基本块，可以由其它基本块经过，
     * 那么当entry等于这个“其它基本块”时，也会将这个后继加入集合
     * 所以最终没有加入集合的基本块，就是block支配的基本块，必须通过block才能到达
     * 与教程给的不同
     * 注意：不能直接递归，否则会爆栈
     * @param entryBlock dfs的入口，调用的时候应当是第一个基本块
     * @param block
     * @param noDominateSet 不被block支配的基本块集合
     */
    private void dfsDominateBlocks(BasicBlock entryBlock, BasicBlock block, Set<BasicBlock> noDominateSet) {
        if (entryBlock == block) {
            return; // 终止条件
        }

        Stack<BasicBlock> stack = new Stack<>();
        stack.push(entryBlock);
        noDominateSet.add(entryBlock); // 标记初始块

        while (!stack.isEmpty()) {
            BasicBlock currentBlock = stack.pop();
            List<BasicBlock> outBlocks = currentBlock.getOutBlocks();

            if (outBlocks != null) {
                for (BasicBlock outBlock : outBlocks) {
                    if (outBlock != block && !noDominateSet.contains(outBlock)) {
                        noDominateSet.add(outBlock); // 标记为已访问
                        stack.push(outBlock);        // 压栈继续处理
                    }
                }
            }
        }
    }

    /**
     * 生成支配树，其实就是求直接支配关系
     */
    public void generateDominantTree(){
        List<BasicBlock> functionBlocks = function.getAllBasicBlocks();
        for(BasicBlock functionBlock : functionBlocks){
            List<BasicBlock> dominateBlocks = functionBlock.getDominateBlocks();
            for(BasicBlock dominateBlock : dominateBlocks){
                //这里需要判断两个基本块是否直接支配
                /**
                  我们希望，让functionBlock直接支配dominate_block
                  只要支配+不相等，那么就是严格支配
                  那么就需要判断，functionBlock支配的节点中，有没有支配dominate_Block的节点
                  如果有，那么说明functionBlock不是直接支配dominate_block
                 */
                boolean flag = (functionBlock != dominateBlock);
                if(flag){
                    for(BasicBlock midBlock:dominateBlocks){
                        if(midBlock != dominateBlock && midBlock != functionBlock){
                            List<BasicBlock> midDominateBlocks = midBlock.getDominateBlocks();
                            if (midDominateBlocks.contains(dominateBlock)) {
                                flag = false;
                                break;
                            }
                        }
                    }
                }
                if(flag){
                    //如果flag是true，说明确实严格支配
                    //需要添加双向边，也就是添加支配树的父节点和子节点
                    //首先必须明确，在这里functionBlock是父节点，dominateBlock是子节点
                    parentBlock.put(dominateBlock, functionBlock);
                    childBlocks.computeIfAbsent(functionBlock, k -> new ArrayList<>()).add(dominateBlock);
                }
            }
        }
    }

    /**
     * 构建支配边界
     * 按照教程给的伪代码遍历即可
     * 首先，遍历函数的所有基本块和后记，记为a和b
     * 令x等于a
     * 然后，当x不严格支配b，将b纳入DF(x)中
     * x更新为x的直接支配者，也就是支配树的父节点
     */
    public void generateDominantEdge(){
        Map<BasicBlock, List<BasicBlock>> CfgOut = function.getCfgGraph().getOutBlocks();
        for(BasicBlock entryBlock : CfgOut.keySet()){
            List<BasicBlock> dominateBlocks = CfgOut.get(entryBlock);
            for(BasicBlock dominateBlock : dominateBlocks){
                //现在开始，遍历流图所有的边，相当于for ⟨a,b⟩ ∈ CFG 图的边集
                BasicBlock x = entryBlock;
                //我们现在需要找一个条件，判断x不严格支配b
                //说白了就两种可能性：要么x和b相等，要么x不支配b(x的支配集不存在b)
                while(x!=null
                        && (x == dominateBlock
                        || !(x.getDominateBlocks().contains(dominateBlock)))){
                    //将b纳入
                    dominateEdge.computeIfAbsent(x, k->new ArrayList<>()).add(dominateBlock);
                    x = x.getParentDominateBlock();
                }
            }
        }
    }

    /**
     * 输出支配树，debug专用
     */
    public String printDominantTree() {
        StringBuilder sb = new StringBuilder();
        List<BasicBlock> blocks = function.getAllBasicBlocks();

        for (BasicBlock block : blocks) {
            if(!block.isExist()) continue;
            sb.append("基本块: ").append(block.getName()).append("\n\t\t\t");

            // 支配集合
            sb.append("支配集合: ").append("\n\t\t\t\t");
            List<BasicBlock> dominateBlocks = block.getDominateBlocks();
            if(dominateBlocks!=null){
                sb.append(block.getDominateBlocks().stream()
                        .map(BasicBlock::getName)
                        .collect(Collectors.joining("\n\t\t\t\t")));

            }else{
                sb.append("没有支配集合");
            }
            sb.append("\n\t\t\t");

            // 支配边
            sb.append("支配边: ").append("\n\t\t\t\t");
            List<BasicBlock> dominateEdges = block.getDominateEdge();
            if(dominateEdges != null){
                sb.append(dominateEdges.stream()
                        .map(BasicBlock::getName)
                        .collect(Collectors.joining("\n\t\t\t\t")));
            }else{
                sb.append("没有支配边");
            }

            sb.append("\n\t\t\t");
            // 支配树父节点
            sb.append("支配树父节点: ").append("\n\t\t\t\t");
            BasicBlock parent = parentBlock.get(block);
            sb.append(parent != null ? parent.getName() : "没有父节点");
            sb.append("\n\t\t\t");

            // 支配树子节点
            sb.append("支配树子节点: ").append("\n\t\t\t\t");
            List<BasicBlock> childDominateBlocks = block.getChildDominateBlocks();
            if(childDominateBlocks!=null){
                sb.append(childDominateBlocks.stream()
                        .map(BasicBlock::getName)
                        .collect(Collectors.joining("\n\t\t\t\t")));

            }else{
                sb.append("没有支配树子节点");
            }
            sb.append("\n\t\t");
        }

        sb.append("\n\t");
        return sb.toString();
    }

    /**
     * 当两个基本块要合并时，将支配树也合并
     * 将后面的基本块合并在前面的基本块
     * 如果需要将前面的基本块合并在后面的基本块，换个调用方向即可
     */
    public void mergeDominantTree(BasicBlock preBlock, BasicBlock postBlock){
        mergeDominantBlocks(preBlock, postBlock);
        mergeDominantEdges(preBlock, postBlock);
        mergeDominantParent(preBlock, postBlock);
        mergeDominantChild(preBlock, postBlock);
    }

    private void mergeDominantBlocks(BasicBlock preBlock, BasicBlock postBlock){
        List<BasicBlock> preDominateBlocks = preBlock.getDominateBlocks();
        List<BasicBlock> postDominateBlocks = postBlock.getDominateBlocks();
        for(BasicBlock postDominateBlock : postDominateBlocks){
            if(!preDominateBlocks.contains(postDominateBlock)){
                preDominateBlocks.add(postDominateBlock);
            }
        }
        dominateBlocks.put(preBlock, preDominateBlocks);
    }

    private void mergeDominantEdges(BasicBlock preBlock, BasicBlock postBlock){
        List<BasicBlock> preDominateEdges = preBlock.getDominateEdge();
        List<BasicBlock> postDominateEdges = postBlock.getDominateEdge();
        for(BasicBlock postDominateBlock : postDominateEdges){
            if(!preDominateEdges.contains(postDominateBlock)){
                preDominateEdges.add(postDominateBlock);
            }
        }
        dominateEdge.put(preBlock, preDominateEdges);
    }

    private void mergeDominantParent(BasicBlock preBlock, BasicBlock postBlock){
        BasicBlock preDominateParent = preBlock.getParentDominateBlock();
        BasicBlock postDominateParent = postBlock.getParentDominateBlock();
        if(preDominateParent!=null && postDominateParent!=null){
            if(!preDominateParent.equals(postDominateParent)){
                parentBlock.put(preBlock, postDominateParent);
            }
        }else if(preDominateParent==null && postDominateParent!=null){
            parentBlock.put(preBlock, postDominateParent);
        }
    }

    private void mergeDominantChild(BasicBlock preBlock, BasicBlock postBlock){
        List<BasicBlock> preDominateChild = preBlock.getChildDominateBlocks();
        List<BasicBlock> postDominateChild = postBlock.getChildDominateBlocks();
        for(BasicBlock postDominateBlock : postDominateChild){
            if(!preDominateChild.contains(postDominateBlock)){
                preDominateChild.add(postDominateBlock);
            }
        }
        childBlocks.put(preBlock, preDominateChild);
    }
}
