package midpass;

import midend.MyModule;
import midend.value.BasicBlock;
import midend.value.Function;
import midend.value.instrs.Branch;
import midend.value.instrs.Instr;
import midend.value.instrs.Jump;
import util.nodelist.NodeList;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;

public class MakeDomTree {
    ArrayList<Function> functions;
    HashMap<BasicBlock, ArrayList<BasicBlock>> prevMap;
    HashMap<BasicBlock, ArrayList<BasicBlock>> successMap;


    public MakeDomTree(ArrayList<Function> functions) {
        this.functions = functions;
        removeUnreachableBlock(); //删去不可达基本块
        getCFG(); //获取每个基本块的前驱和后继
        getDOM(); //获取基本块之间的支配关系
        getIDom(); // 获取基本块之间的直接支配关系
        getDominanceFrontier(); // 获取支配边界
        getDomTree(); //由直接支配关系获取支配树
    }

    public void getDomTree() {
        for (Function function : functions) {
            funcGetDomTree(function);
        }
    }

    public void funcGetDomTree(Function function) {
        BasicBlock entry = function.getFirstBlock();
        //每个点只可能被一个点直接支配
        buildTree(entry, 1);
    }

    public void buildTree(BasicBlock start, int layer) {
        start.setLayerInDomTree(layer);
        for (BasicBlock basicBlock : start.getIdomBB()) {
            basicBlock.setFatherDominator(start); //子节点指向父节点
            buildTree(basicBlock, layer + 1);
        }
    }

    public void getDominanceFrontier() {
        for (Function function : functions) {
            funcGetDominanceFrontier(function);
        }
    }

    public void funcGetDominanceFrontier(Function function) {
        for (var node : function.getBasicBlocks()) {
            BasicBlock basicBlock = node.get();
            HashSet<BasicBlock> DF = new HashSet<>();
            for (var n : function.getBasicBlocks()) {
                BasicBlock block = n.get();
                if (xDFy(basicBlock, block)) {
                    DF.add(block);
                }
            }
            basicBlock.setDfBB(DF);
        }
    }

    public boolean xDFy(BasicBlock x, BasicBlock y) {
        //定义：x支配y的前驱但是x不严格支配y
        for (BasicBlock prev : y.getPrevBB()) {
            if (x.getDomBB().contains(prev) && (x.equals(y) || !x.getDomBB().contains(y))) {
                return true;
            }
        }
        return false;
    }

    public void getIDom() {
        //获取直接支配关系，从定义出发
        for (Function function : functions) {
            funcGetIDom(function);
        }
    }

    public void funcGetIDom(Function function) {
        for (var node : function.getBasicBlocks()) {
            BasicBlock basicBlock = node.get();
            HashSet<BasicBlock> iDominate = new HashSet<>();
            for (BasicBlock block : basicBlock.getDomBB()) {
                if (AIDominanceB(basicBlock, block)) {
                    iDominate.add(block);
                }
            }
            basicBlock.setIdomBB(iDominate);
        }
    }

    public boolean AIDominanceB(BasicBlock A, BasicBlock B) {
        //A 直接支配 B 的条件是 A是距离B最近的严格支配B的节点
        HashSet<BasicBlock> ADom = A.getDomBB();
        if (A.equals(B)) {
            return false;
        }
        if (!ADom.contains(B)) {
            return false;
        }
        for (BasicBlock block : ADom) {
            //如果A不直接支配B，而A又支配B，则A支配的节点中必有一节点严格支配B
            if (!block.equals(A) && !block.equals(B)) {
                if (block.getDomBB().contains(B)) {
                    return false;
                }
            }
        }
        //以上条件都不成立时，A直接支配B
        return true;
    }

    public void funcGetDOM(Function function) {
        //获取节点的支配关系
        BasicBlock entry = function.getFirstBlock();

        for (var node : function.getBasicBlocks()) {
            BasicBlock basicBlock = node.get();
            HashSet<BasicBlock> dominate = new HashSet<>();
            HashSet<BasicBlock> visited = new HashSet<>();
            dfs(entry, basicBlock, visited);

            for (var n: function.getBasicBlocks()) {
                BasicBlock block = n.get();
                if (!visited.contains(block)) {
                    // 不经过basicBlock就访问不到，说明basicBlock支配block
                    dominate.add(block);
                }
            }

            basicBlock.setDomBB(dominate);
        }
    }

    public void dfs(BasicBlock start, BasicBlock stop, HashSet<BasicBlock> visited) {
        if (start.equals(stop)) {
            return;
        }
        if (visited.contains(start)) {
            return;
        }
        visited.add(start);
        for (BasicBlock basicBlock : start.getSucBB()) {
            if (!visited.contains(basicBlock) && !basicBlock.equals(stop)) {
                dfs(basicBlock, stop, visited);
            }
        }
    }

    public void getDOM() {
        //获取支配关系
        for (Function function : functions) {
            funcGetDOM(function);
        }
    }

    public void getCFG() {
        for (Function function : functions) {
            funcGetCFG(function);
        }
    }

    public void funcGetCFG(Function function) {
        getPrevSuc(function);
        //将前驱及后继集合分别写入block
        for (var node : function.getBasicBlocks()) {
            BasicBlock basicBlock = node.get();
            basicBlock.setPrevBB(prevMap.get(basicBlock));
            basicBlock.setSucBB(successMap.get(basicBlock));
        }
    }

    public void removeUnreachableBlock() {
        for (Function function : functions) {
            funcRemoveUnreachableBlock(function);
        }
    }

    public void funcRemoveUnreachableBlock(Function function) {
        getPrevSuc(function);

        //去掉没有前驱的点，算法使用DFS，从入口开始dfs，最终没有被访问到的就是不可达基本块
        HashSet<BasicBlock> visitedBlock = new HashSet<>();
        dfs_visit(function.getFirstBlock(), visitedBlock);

        for (var basicBlock : function.getBasicBlocks()) {
            if (!visitedBlock.contains(basicBlock.get())) {
                basicBlock.remove();
                basicBlock.get().removeAllInstr();
            }
        }
    }

    public void getPrevSuc(Function function) {
        NodeList<BasicBlock> basicBlocks = function.getBasicBlocks();
        prevMap = new HashMap<>();
        successMap = new HashMap<>();

        for (var basicBlock : basicBlocks) {
            prevMap.put(basicBlock.get(), new ArrayList<>());
            successMap.put(basicBlock.get(), new ArrayList<>());
        }

        for (var block : basicBlocks) {
            BasicBlock curBlock = block.get();
            Instr lastInstr = curBlock.getLastInstr();
            if (lastInstr instanceof Branch) {
                BasicBlock trueBB = (BasicBlock) lastInstr.getOperandList().get(1);
                BasicBlock falseBB = (BasicBlock) lastInstr.getOperandList().get(2);
                successMap.get(curBlock).add(trueBB);
                successMap.get(curBlock).add(falseBB);
                prevMap.get(trueBB).add(curBlock);
                prevMap.get(falseBB).add(curBlock);
            } else if (lastInstr instanceof Jump) {
                BasicBlock targetBB = (BasicBlock) lastInstr.getOperandList().get(0);
                successMap.get(curBlock).add(targetBB);
                prevMap.get(targetBB).add(curBlock);
            }
        }
    }

    public void dfs_visit(BasicBlock start, HashSet<BasicBlock> visitedBlock) {
        if (visitedBlock.contains(start)) {
            return;
        }
        visitedBlock.add(start);
        for (BasicBlock basicBlock : successMap.get(start)) {
            dfs_visit(basicBlock, visitedBlock);
        }
    }
}
