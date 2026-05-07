package ir.pass.analyze;
import ir.value.*;
import ir.value.Module;
import ir.value.user.Function;
import tools.BlockAnalyzer;

import java.util.ArrayList;
import java.util.BitSet;
import java.util.LinkedHashMap;


public class DomAnalyzer{

    private static  ArrayList<Integer>[] graph1;
    private static ArrayList<Integer>[] graph2;
    private static  int[] dfn ;
    public static void calDomForModule(Module module){
        DomAnalyzer.calPredSuccBB(module);
        for(Function function:module.functions.values()){
            if(function.blocks.size()>1) calDomForFunction(function);
            else if(function.blocks.size()==1){
                var block = function.blocks.get(0);
                block.getDominate().clear();
                block.getDominate().add(block);
                block.getDominatedBy().clear();
                block.getDominatedBy().add(block);
            }
        }
    }


    public static void calPredSuccBB(Module module){
        BlockAnalyzer.updateBlockPredAndSuccs(module);
    }
    public static long time0=0;

    public static double gettime(){
        long timenow = System.currentTimeMillis();
        double returntime = (timenow-time0)/1000.0;
        time0 = timenow;
        return returntime;
    }
    public static void calDomForFunction(Function function){
        gettime();
//        System.out.printf("||domRelation:%f s||", gettime());
        calDomRelation(function,true);
        calDomFrontier(function);
//        System.out.printf("domFrontier: %f %n", gettime());
    }

    public static void calDepthForModule(Module module){
        for(var function:module.functions.values()){
            if(function.blocks.size()!=0) {
                calDomDepthForFunction(function);
            }
        }
    }

    public static void calDomDepthForFunction(Function function){
        dfs(0,function.blocks.get(0));
    }


    public static void dfs(int domDepth,BasicBlock basicBlock){
        basicBlock.setDomDepth(domDepth);
        for(var BB:basicBlock.getIdominate()){
            dfs(domDepth+1,BB);
        }
    }
    public static void dfs2(BasicBlock BB){
        if(BB.getIdominate().size()==0){
            BB.getDominatedBy().add(BB);
            BB.getDominate().add(BB);
        }
        else{
            for(var idomBB:BB.getIdominate()){
                dfs2(idomBB);
                BB.getDominate().addAll(idomBB.getDominate());
            }
            BB.getDominate().add(BB);
            for(var domBB:BB.getDominate()){
                domBB.getDominatedBy().add(BB);
            }
        }
    }
    /*
        Dom(0) = {0}
        for i = 1 to n
            Dom(i) = {0,1,2,3,....n}
        changed = true
        while(changed){
            for i = 1 to n
                tmp = {i} union { the intersection of all Dom(j) only if j in preds(i) }
                if tmp != Dom(i) then
                    Dom(i) = tmp
                    Changed = true
        }
    */

    public static void calDomRelation(Function function, boolean opt){
        var bb2index = new LinkedHashMap<BasicBlock, Integer>();
        var index2bb = new ArrayList<BasicBlock>();
        if (opt) {
            //采用Lengauer-Tarjan算法进行优化
            index2bb.add(new BasicBlock(null,null,false));
            int index = 1;
            for (var bb : function.blocks) {
                bb.domClear();
                bb2index.put(bb, index);
                index2bb.add(bb);
                index++;
            }
            int paramM = 0;
            for(var bb : function.blocks) {
                paramM += bb.getSuccs().size();
            }
            //初始化
            DomTarjan.init(index,paramM);
//            System.out.println("!!!!!");
            //加边
            for(var bb : function.blocks) {
                for(var succBB : bb.getSuccs()){
                    DomTarjan.add(bb2index.get(bb),bb2index.get(succBB),0);
                    DomTarjan.add(bb2index.get(succBB),bb2index.get(bb),1);
                }
            }
            DomTarjan.run();
            int cnt = DomTarjan.getCnt();
            int [] ord = DomTarjan.getOrd();
            int [] idom = DomTarjan.getIdom();
            //支配树已建立,求出idom
            for(int i=cnt;i>=2;--i){
                index2bb.get(ord[i]).setIdominatedBy(index2bb.get(idom[ord[i]]));
                index2bb.get(idom[ord[i]]).getIdominate().add(index2bb.get(ord[i]));
            }
            //TODO:考虑对支配树进行一次DFS,利用idom关系得到dom关系,一个节点能支配它在支配树中所有子树节点
            dfs2(function.blocks.get(0));
        }
        else {
            var doms = new ArrayList<BitSet>();
            var tmp = new BitSet();
            tmp.set(0);
            doms.add(tmp);
            for (int i = 1; i < function.blocks.size(); i++) {
                tmp = new BitSet();
                tmp.set(0, function.blocks.size() - 1);
                doms.add(tmp);
            }
            int index = 0;
            for (var bb : function.blocks) {
                bb.domClear();
                bb2index.put(bb, index);
                index2bb.add(bb);
                index++;
            }
            boolean changed = true;
            while (changed) {
                changed = false;
                for (var bb : function.blocks) {
                    //tmp = new BitSet();
                    tmp = doms.get(bb2index.get(bb));
                    boolean isFirst = true;
                    for (var predBB : bb.getPreds()) {
                        if (isFirst) {
                            tmp = (BitSet) doms.get(bb2index.get(predBB)).clone();
                            isFirst = false;
                        } else {
                            tmp.and(doms.get(bb2index.get(predBB)));
                        }
                    }
                    tmp.set(bb2index.get(bb));
                    if (!tmp.equals(doms.get(bb2index.get(bb)))) {
                        doms.set(bb2index.get(bb), tmp);
                        changed = true;
                    }
                }
            }
            //update the info to bb
            for (int i = 0; i < function.blocks.size(); i++) {
                BasicBlock bb = index2bb.get(i);
                var domer = doms.get(i).nextSetBit(0);
                while (domer != -1) {
                    bb.getDominatedBy().add(index2bb.get(domer));
                    domer = doms.get(i).nextSetBit(domer + 1);
                }
            }
            for (var bb : function.blocks) {
                for (var domBB : bb.getDominatedBy()) {
                    domBB.getDominate().add(bb);
                }
            }
            //calculate idom
            for (int i = 0; i < function.blocks.size(); i++) {
                var curBB = index2bb.get(i);
                for (var possibleIdomBB : curBB.getDominatedBy()) {
                    if (possibleIdomBB != curBB) {
                        boolean isIdom = true;
                        for (var domerBB : curBB.getDominatedBy()) {
                            if (domerBB != curBB && domerBB != possibleIdomBB && domerBB.getDominatedBy().contains(possibleIdomBB)) {
                                isIdom = false;
                                break;
                            }
                        }
                        if (isIdom) {
                            curBB.setIdominatedBy(possibleIdomBB);
                            possibleIdomBB.getIdominate().add(curBB);
                            break;
                        }
                    }
                }

            }
        }
    }
    /*
       for x : blocks:
           DF(x) = None
       for x : blocks:
           if x.preds.size() > 1:
                for p : preds:
                runner = p
                while runner != idom(x)
                    DF(runner) = DF(runner) union {x}
                    runner = idom(runner)
    */
    public static void calDomFrontier(Function function){
        for(var block:function.blocks){
            block.getDomFrontier().clear();
        }
        for(var block:function.blocks){
            for(var succBB:block.getSuccs()){
                var tmp = block;    //其实就是遍历cfg边(block->succBB)，只要block不能决定succBB，block的支配边界中，并用支配树上block的父节点更新block
                while(tmp != null && (tmp==succBB||!succBB.getDominatedBy().contains(tmp))){
                    tmp.getDomFrontier().add(succBB);
                    tmp = tmp.getIdominatedBy();
                }
            }
        }
    }

    public static class DomTarjan {
        static int n, m;
        static int[] ans;
        static int tot;
        static int[][] fst;
        static int[] nxt;
        static int[] to;
        static int[] dfn;
        static int[] ord;



        static int cnt;
        static int[] fth;
        static int[] idom;
        static int[] semi;
        static int[] uni;
        static int[] mn;


        public static int getCnt() {
            return cnt;
        }

        public static int[] getOrd() {
            return ord;
        }

        public static int[] getIdom() {
            return idom;
        }
        static void init(int paramN,int paramM){
            //fst,nxt和to数组用于建图
            n = paramN;
            m = paramM;
//            System.out.println(n);
//            System.out.println(m);
            ans = new int[n+1];
            fst = new int[n+1][3];;
            nxt = new int[m+m+n+1];
            to = new int[m+m+n+1];;
            dfn = new int[n+1];
            ord = new int[n+1];
            fth = new int[n+1];
            idom = new int[n+1];
            semi = new int[n+1];
            uni = new int[n+1];
            mn = new int[n+1];
            cnt = 0;
            tot = 0;
        }
        static void add(int u, int v, int id) {
            nxt[++tot] = fst[u][id];
            to[tot] = v;
            fst[u][id] = tot;
        }

        static void Tarjan(int u) {
            ord[dfn[u] = ++cnt] = u;
            for (int i = fst[u][0]; i != 0; i = nxt[i]) {
                int v = to[i];
                if (dfn[v] == 0) {
                    fth[v] = u;
                    Tarjan(v);
                }
            }
        }

        static int uni_query(int u) {
            if (u == uni[u])
                return u;
            int tmp = uni_query(uni[u]);
            if (dfn[semi[mn[u]]] > dfn[semi[mn[uni[u]]]])
                mn[u] = mn[uni[u]];
            return uni[u] = tmp;
        }

        static void Lengauer_Tarjan(int s) {
            Tarjan(s);
            for (int i = 1; i <= n; ++i)
                semi[i] = uni[i] = mn[i] = i;
            for (int id = cnt; id >= 2; --id) {
                int u = ord[id];
                for (int i = fst[u][1]; i != 0; i = nxt[i]) {
                    int v = to[i];
                    if (dfn[v] == 0)
                        continue;
                    uni_query(v);
                    if (dfn[semi[u]] > dfn[semi[mn[v]]])
                        semi[u] = semi[mn[v]];
                }
                uni[u] = fth[u];
                add(semi[u], u, 2);
                u = fth[u];
                for (int i = fst[u][2]; i != 0; i = nxt[i]) {
                    int v = to[i];
                    uni_query(v);
                    idom[v] = (u == semi[mn[v]]) ? u : mn[v];
                }
                fst[u][2] = 0;
            }
            for (int i = 2; i <= cnt; ++i) {
                int u = ord[i];
                if (idom[u] != semi[u])
                    idom[u] = idom[idom[u]];
            }
        }

        public static void run() {
            Lengauer_Tarjan(1);
        }
    }


}