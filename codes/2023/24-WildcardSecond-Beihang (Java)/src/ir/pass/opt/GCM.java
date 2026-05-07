package ir.pass.opt;

import ir.Value;
import ir.instr.*;
import ir.pass.analyze.LoopAnalyzer;
import ir.value.BasicBlock;
import ir.value.Module;
import ir.pass.analyze.DomAnalyzer;
import ir.value.user.Function;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;

public class GCM implements Pass {
    //as same in the essay by Cliff Click
    private final LinkedHashSet<Instr> visit = new LinkedHashSet<>();
    private final LinkedHashMap<Instr, BasicBlock> block = new LinkedHashMap<>();

    private Function function;

    @Override
    public void run(Module module) {
        //重构CFG,重建支配树,计算每个基本块的支配树深度和循环树深度
        //DomAnalyzer.calPredSuccBB(module);这句话转移到calDom中
        DomAnalyzer.calDomForModule(module);
        DomAnalyzer.calDepthForModule(module);
        LoopAnalyzer.analyzeLoop(module);
        for (var function : module.functions.values()) {
            if (function.blocks.size() != 0) {
                gcmForFunction(function);
            }
        }
    }

    /*
    for i in all instructions
        if pinned(i)
           visit(i) = true
           for x in inputs
               schedule_early(x)
    for i in all instructions
        if pinned(i)
           visit(i) = true
           for x in users
               schedule_late(x)
    */
    public void gcmForFunction(Function function) {
//        for(var block:function.blocks){
//            block.printIdominate();
//        }
//        for(var block:function.blocks){
//            block.printIdominatedBy();
//        }
        this.function = function;
        //pre work
        for (var BB : function.blocks) {
            for (var instr : BB.getInsts()) {
                block.put((Instr) instr, BB);
            }
        }
        for (var BB : function.blocks) {
            for (var instr : BB.getInsts()) {
                scheduleEarly((Instr) instr);
            }
        }
        visit.clear();
        for (int i = function.blocks.size() - 1; i >= 0; --i) {
            var BB = function.blocks.get(i);
            ArrayList<Value> instrs = new ArrayList<>(BB.getInsts());
            for (int j = instrs.size() - 1; j >= 0; --j) {
                var instr = instrs.get(j);
                scheduleLate((Instr) instr);
            }
        }

    }

    /*
    if visit(i) return
    visit(i) = true
    block(i) = root
    for all inputs x:
        schedule_early(x)
        if i.block.dom_depth < x.block.dom_depth
            i.block = x.block
    */
    public void scheduleEarly(Instr instr) {
        if (visit.contains(instr) || isPinned(instr)) {
            return;
        }
        visit.add(instr);
        block.put(instr, function.blocks.get(0));
        for (var operand : instr.getOperands()) {
            if (operand instanceof Instr) {
                scheduleEarly((Instr) operand);
                if (block.get(instr).getDomDepth() < block.get((Instr) operand).getDomDepth()) {
                    block.put(instr, block.get((Instr) operand));
                }
            }
        }
    }

    /*
    if visit(i) return
    visit(i) = true
    Block lca = null
    for all uses y of i
        schedule_late(y)
        Block use = block(y)
        if y is PHI
            Reverse dependence edge from i to y
            Pick j so that the jth input of y is i
            Use matching block from CFG
            use = block(y).pred[j]
        lca = LCA(lca,use)
    */
    public void scheduleLate(Instr instr) {
        if (visit.contains(instr) || isPinned(instr)) {
            return;
        }
        visit.add(instr);
        BasicBlock lca = null;
        for (var user : instr.getUsers()) {
            scheduleLate((Instr) user);
            BasicBlock use = block.get((Instr) user);
            if (user instanceof Phi) {
                for (var entry : ((Phi) user).getValues().entrySet()) {
                    if (entry.getValue() == instr) {
                        use = entry.getKey();
                        lca = LCA(lca, use);
                    }
                }
            } else {
                lca = LCA(lca, use);
            }
        }
        if (lca == null) {
            lca = block.get(instr);
        }
        if (block.get(instr) == null) {
            System.out.println(instr.toString());
            System.out.println("fuck");
        }
        /*
        Block best = lca
            while lca != i.block
            if lca.loop_nest < best.loop_nest then
                best = lca
            lca  = lca.immediate_dominator
        block(i) = best
        */

        BasicBlock best = lca;
        while (lca != block.get(instr)) {
            if (lca.getLoopDepth() < best.getLoopDepth()) {
                best = lca;
            }
            if (lca.getIdominatedBy() == null) {
                break;
            }
            lca = lca.getIdominatedBy();
        }
        if (lca != null && lca == block.get(instr)) {
            if (lca.getLoopDepth() < best.getLoopDepth()) {
                best = lca;
            }
        }
        block.put(instr, best);
        instrMotion(instr, best);
    }

    public void instrMotion(Instr instr, BasicBlock best) {
        //保证在best进行插入是合法的
        instr.getParent().getInsts().remove(instr);
        instr.setParent(best);
        //遍历best基本块,寻找合法位置
        int late = best.getInsts().size() - 1;
        for (var user : instr.getUsers()) {
            if (user instanceof Instr && (!(user instanceof Phi))) {
                var t = best.getInsts().indexOf(user);
                if (t != -1) {
                    late = Math.min(late, t);
                }
            }
        }
        best.getInsts().add(late, instr);
    }

    //Least Common Ancestors 最近公共祖先,这里应该是离线算法,思路比较简单,就是先跳到一个高度上
    //在同一深度继续往上找,最后一定会相等,最恶劣情况是一根链到根节点,此处可以考虑树上倍增优化
    private BasicBlock LCA(BasicBlock x, BasicBlock y) {
        if (x == null) {
            return y;
        }
        while (x.getDomDepth() > y.getDomDepth()) {
            if (x.getIdominatedBy() == null) {
                return x;
            }
            x = x.getIdominatedBy();
        }
        while (y.getDomDepth() > x.getDomDepth()) {
            if (y.getIdominatedBy() == null) {
                return y;
            }
            y = y.getIdominatedBy();
        }
//        System.out.println(x.toString());
//        System.out.println(y.toString());
        while (x != y) {
            x = x.getIdominatedBy();
            y = y.getIdominatedBy();
        }
        return x;
    }

    public boolean isPinned(Instr instr) {
        return instr instanceof Phi || instr instanceof Br || instr instanceof Ret ||
            instr instanceof Call|| instr instanceof Load ||
            instr instanceof Store;
    }
}
