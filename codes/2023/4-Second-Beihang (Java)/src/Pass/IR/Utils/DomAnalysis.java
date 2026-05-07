package Pass.IR.Utils;

import IR.Value.BasicBlock;
import IR.Value.Function;
import Utils.DataStruct.IList;

import java.util.*;

import static Pass.IR.Utils.UtilFunc.makeCFG;


public class DomAnalysis{
    public static void run(Function function){
        //  dom记录每个块被哪些块支配
        //  idoms记录每个Bb直接支配哪些块

        //  先建立CFG图(Control Flow Graph)
        makeCFG(function);
        //  根据CFG图初始化数据结构
        //  先计算有效的所有block
        removeUselessBlocks(function);
        //  正式开始计算
        runDomForFunc(function);
        runIDomForFunc(function);
        //  建立支配树
        //  根据直接支配关系(idoms) dfs建立支配树
        buildDomTree(function.getBbEntry(), 0);
        //  计算DF
        runDF(function);
    }

    private static void runDomForFunc(Function function){
        BasicBlock enter = function.getBbEntry();
        HashMap<BasicBlock, HashSet<BasicBlock>> dom = new HashMap<>();
        for (IList.INode<BasicBlock, Function> bbNode : function.getBbs()) {
            dom.put(bbNode.getValue(), new HashSet<>());
        }

        for (IList.INode<BasicBlock, Function> bbNode : function.getBbs()) {
            BasicBlock bb = bbNode.getValue();
            HashSet<BasicBlock> know = new HashSet<>();
            dfs(enter, bb, know);

            for (IList.INode<BasicBlock, Function> tempNode : function.getBbs()) {
                BasicBlock temp = tempNode.getValue();
                if (!know.contains(temp)) {
                    dom.get(temp).add(bb);
                }
            }

        }

        function.setDomer(dom);
    }

    private static void dfs(BasicBlock bb, BasicBlock not, HashSet<BasicBlock> know) {
        if (bb.equals(not)) {
            return;
        }
        if (know.contains(bb)) {
            return;
        }
        know.add(bb);
        for (BasicBlock next: bb.getNxtBlocks()) {
            if (!know.contains(next) && !next.equals(not)) {
                dfs(next, not, know);
            }
        }
    }


    private static void runIDomForFunc(Function function){
        HashMap<BasicBlock, ArrayList<BasicBlock>> idoms = new HashMap<>();

        for(IList.INode<BasicBlock, Function> bbNode : function.getBbs()){
            idoms.put(bbNode.getValue(), new ArrayList<>());
        }

        //  计算直接支配(idom)关系
        //  直接支配关系我们直接按定义来计算
        //  严格支配bb，且不严格支配任何(严格支配bb的节点)的节点
        HashMap<BasicBlock, HashSet<BasicBlock>> domer = function.getDomer();
        for (BasicBlock bb : domer.keySet()) {
            HashSet<BasicBlock> tmpDomSet = domer.get(bb);
            for (BasicBlock mayIDom : tmpDomSet) {
                //  严格支配bb
                if (mayIDom.equals(bb)) {
                    continue;
                }

                boolean isIDom = true;
                for (BasicBlock tmpDomBlock : tmpDomSet) {
                    //  tmpDomBlock严格支配bb的节点 && mayIDom严格支配tmpDomBlock
                    //  的话说明它不直接支配bb
                    if (!tmpDomBlock.equals(bb) &&
                            !tmpDomBlock.equals(mayIDom) &&
                            domer.get(tmpDomBlock).contains(mayIDom)) {
                        isIDom = false;
                        break;
                    }
                }

                if (isIDom) {
                    idoms.get(mayIDom).add(bb);
                    break;
                }
            }
        }

        function.setIdoms(idoms);
    }

    private static void removeUselessBlocks(Function function){
        Queue<BasicBlock> q = new LinkedList<>();
        ArrayList<BasicBlock> allBlocks = new ArrayList<>();
        HashSet<BasicBlock> allBlocksSet = new HashSet<>();
        q.add(function.getBbEntry());
        allBlocks.add(function.getBbEntry());
        allBlocksSet.add(function.getBbEntry());
        while (!q.isEmpty()){
            BasicBlock nowBb = q.poll();
            for(BasicBlock nxtBb : nowBb.getNxtBlocks()){
                if(!allBlocksSet.contains(nxtBb)){
                    q.add(nxtBb);
                    allBlocks.add(nxtBb);
                    allBlocksSet.add(nxtBb);
                }
            }
        }

        //  顺便把没用的block删了
        ArrayList<BasicBlock> deletedBlock = new ArrayList<>();
        for (IList.INode<BasicBlock, Function> bbNode : function.getBbs()){
            BasicBlock bb = bbNode.getValue();
            if(!allBlocksSet.contains(bb)){
                deletedBlock.add(bb);
            }
        }
        for(BasicBlock bb : deletedBlock){
            bb.removeSelf();
        }
    }


    private static void runDF(Function function){
        //  数据结构初始化
        HashMap<BasicBlock, ArrayList<BasicBlock>> df = new HashMap<>();
        for (IList.INode<BasicBlock, Function> bbNode : function.getBbs()) {
            df.put(bbNode.getValue(), new ArrayList<>());
        }

        ArrayList<BasicBlock> domPostOrder = getDomPostOrder(function);
        for(BasicBlock X : domPostOrder){
            df.put(X, new ArrayList<>());
            //  DF_local(X)
            for(BasicBlock Y : X.getNxtBlocks()){
                if(Y.getIdominator() != X){
                    df.get(X).add(Y);
                }
            }
            //  DF_up(Z)
            for(BasicBlock Z : X.getIdoms()){
                for(BasicBlock Y : df.get(Z)){
                    if(Y.getIdominator() != X){
                        df.get(X).add(Y);
                    }
                }
            }
        }

        function.setDF(df);
    }

    public static ArrayList<BasicBlock> getDomPostOrder(Function function){
        ArrayList<BasicBlock> postOrder = new ArrayList<>();
        Stack<BasicBlock> stack = new Stack<>();
        HashSet<BasicBlock> visitedAllNxtBbs = new HashSet<>();

        stack.add(function.getBbEntry());
        while (!stack.empty()){
            BasicBlock nowBb = stack.peek();
            if(visitedAllNxtBbs.contains(nowBb)){
                postOrder.add(nowBb);
                stack.pop();
                continue;
            }

            for(BasicBlock sonBb : nowBb.getIdoms()){
                stack.push(sonBb);
            }

            visitedAllNxtBbs.add(nowBb);
        }
        return postOrder;
    }

    private static void buildDomTree(BasicBlock bb, int domLV){
        bb.setDomLV(domLV);
        for(BasicBlock sonBb : bb.getIdoms()){
            buildDomTree(sonBb, domLV + 1);
        }
    }
}
