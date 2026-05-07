package ir.pass.opt;

import ir.Value;
import ir.instr.Alloca;
import ir.instr.Bitcast;
import ir.instr.Call;
import ir.instr.GetElementPtrInst;
import ir.instr.Global;
import ir.instr.Instr;
import ir.instr.Load;
import ir.instr.Phi;
import ir.instr.Store;
import ir.pass.analyze.AccessAnalyzer;
import ir.type.IntType;
import ir.value.Arg;
import ir.value.BasicBlock;
import ir.value.ConstNumber;
import ir.value.Module;
import ir.value.User;
import ir.value.user.Function;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;

public class ArrayEliminate {
    //必须在GVN之后才能运行，多级GetElementPtr需要合并,相同ptr来源也必须替换成同一个，否则会导致错误！
    LinkedHashMap<BasicBlock, ArrayInfo> blockArrayInfos;
    LinkedHashMap<Store, Integer> storeCount;
    LinkedHashSet<Value> ignoredArray;

    boolean needrepeat;

    public static long time0=0;

    public static double gettime(){
        long timenow = System.currentTimeMillis();
        double returntime = (timenow-time0)/1000.0;
        time0 = timenow;
        return returntime;
    }
    public boolean run(Module module) {
        needrepeat = false;
        ignoredArray = new LinkedHashSet<Value>();
        sideEffectAnalyze(module);
//
//        InlineFunction inlineFunction = new InlineFunction();
//        inlineFunction.newBiCallGraph(module);
//        callees = inlineFunction.callees;
//        callers = inlineFunction.callers;

        blockArrayInfos = new LinkedHashMap<>();
        storeCount = new LinkedHashMap<>();
        module.functions.values().forEach(f -> f.blocks.forEach(block -> {
            blockArrayInfos.put(block, new ArrayInfo());
        }));
        gettime();
        AccessAnalyzer.run(module);
        System.out.println("可达性计算耗时："+String.format("%f", gettime()));
        module.functions.values().forEach(this::runForFunction);
        if(!needrepeat){
            ArrayList<Instr> needRemove = new ArrayList<>();
            storeCount.keySet().forEach(s -> {
                if(s.getDest() instanceof GetElementPtrInst){
                    if (storeCount.get(s) == 0 && !(((GetElementPtrInst) s.getDest()).getOP(0) instanceof Arg)) {
                        needRemove.add(s);
                    }
                }
            });
            module.functions.values().forEach(func -> {
                for (var block : func.blocks) {
                    block.getInsts().removeAll(needRemove);
                }
            });
            needRemove.forEach(User::delete);
            if (needRemove.size() > 0) {
                needrepeat = true;
            }
        }
        return needrepeat;
    }

    private void sideEffectAnalyze(Module module) {
        LinkedHashSet<Value> visited = new LinkedHashSet<>();
        for (var func : module.functions.values()) {
            LinkedHashSet<Value> visitedOfCur = new LinkedHashSet<>();
            for (BasicBlock block : func.blocks) {
                for (Value inst : block.getInsts()) {
                    if (inst instanceof GetElementPtrInst) {
                        visitedOfCur.add(((GetElementPtrInst) inst).getOP(0));
                    }
                    if(inst instanceof Call && !((Call) inst).getFunction().blocks.isEmpty()){
                        for(var ope : ((Call) inst).getOperands()){
                            if(ope instanceof GetElementPtrInst){
                                ignoredArray.add(((GetElementPtrInst) ope).getOP(0));
                            }
                        }
                    }
                    if(inst instanceof Bitcast && ((Bitcast) inst).crossCast){
                        ignoredArray.add(((Bitcast) inst).getOP(0));
                        ignoredArray.add(inst);
                    }
                }
            }
            for (var arr : visitedOfCur) {
                if (!visited.contains(arr)) {
                    visited.add(arr);
                } else {
                    ignoredArray.add(arr);
                }
            }
        }
    }

    public static boolean ptrConflit(GetElementPtrInst a, Value b0) {
        if(b0 instanceof ConstNumber){
            return true;
        }
        if (b0 instanceof Arg) {
            return a.getOP(0).equals(b0);
        }
        if(b0 instanceof Bitcast && ((Bitcast) b0).crossCast){
            return ptrConflit(a, ((Bitcast) b0).getOP(0));
        }
        GetElementPtrInst b = (GetElementPtrInst) b0;
        if (a.getOP(0) != b.getOP(0)) {
            return false;
        }
        if (a.getOP(0) instanceof Arg) {
            int i = 1;
            boolean checked = false;
            for (; i < a.getOperands().size() && i < b.getOperands().size() - 1; i++) {
                checked = true;
                if (!(a.getOP(i) instanceof ConstNumber && b.getOP(i) instanceof ConstNumber)) {
                    return true;
                }
                if ((((ConstNumber) a.getOP(i)).value).equals(((ConstNumber) b.getOP(i)).value)) {
                    return true;
                }
            }
            //这意味着传入的是整个数组或者没有冲突。传入整个数组自然有冲突。
            return !checked;
        } else {
            int i = 2;
            boolean checked = false;

            for (; i < a.getOperands().size() && i < b.getOperands().size() - 1; i++) {
                checked = true;
                if (!(a.getOP(i) instanceof ConstNumber && b.getOP(i) instanceof ConstNumber)) {
                    return true;
                }
                if ((((ConstNumber) a.getOP(i)).value).equals(((ConstNumber) b.getOP(i)).value)) {
                    return true;
                }
            }
            return !checked;
        }
    }

    private boolean mustPass(BasicBlock from, BasicBlock to, BasicBlock mid) {
        assert !mid.equals(to);
        return (mid.getDominate().contains(to) && from.getDominate().contains(to) &&
            from.getDominate().contains(mid));
    }

    public static boolean mayConflict(GetElementPtrInst p1, GetElementPtrInst p2) {
        if(p1.getOP(0) instanceof Bitcast || p2.getOP(0) instanceof Bitcast){
            return true;
        }
        if(!p1.getOP(0).equals(p2.getOP(0)))return false;
        for (int i = 1; i < p1.getOperands().size() && i < p2.getOperands().size(); i++) {
            if (p1.getOP(i) instanceof ConstNumber && p2.getOP(i) instanceof ConstNumber) {
                if (!((ConstNumber) p1.getOP(i)).value.equals(((ConstNumber) p2.getOP(i)).value)) {
                    return false;
                }
            }
        }
        return true;
    }

    private boolean analyzeForBlock(BasicBlock block, Function function) {

        boolean isloop = false;
        var info = blockArrayInfos.get(block);
        for (var inst : block.getInsts()) {
            if (inst instanceof Store) {
                if (!(((Store) inst).getOP(0) instanceof GetElementPtrInst)) {
                    continue;
                }
                if (ignoredArray.contains(
                    ((GetElementPtrInst) ((Store) inst).getDest()).getOP(0))) {
                    continue;
                }
                info.memout.put((GetElementPtrInst) ((Store) inst).getDest(), (Store) inst);
                storeCount.put((Store) inst, 0);
                info.mems.add((Instr) inst);
            } else if (inst instanceof Call) {
                if (((Call) inst).getFunction().name.equals("memset") ||
                    ((Call) inst).getFunction().name.equals("putarray") ||
                    ((Call) inst).getFunction().name.equals("putfarray")) {
                    info.mems.add((Instr) inst);
                    continue;
                }
                for (var ope : ((Call) inst).getOperands()) {
                    if (ope instanceof GetElementPtrInst || ope instanceof Arg) {
                        info.memFuncsout.add(ope);
                    }
                }
                if(((Call) inst).getFunction().name.equals(function.name)){
                    info.memFuncsout.add(new ConstNumber(0));
                    isloop = true;
                }
                info.mems.add((Instr) inst);
            } else if (inst instanceof Load) {
                if (!(((Load) inst).getOP(0) instanceof GetElementPtrInst)) {
                    continue;
                }
                if (ignoredArray.contains(
                    ((GetElementPtrInst) ((Load) inst).getTarget()).getOP(0))) {
                    continue;
                }
                info.mems.add((Load) inst);
            }
        }
        return isloop;
    }


    private void runForFunction(Function function) {
        if (function.blocks.isEmpty()) {
            return;
        }
        boolean hasLoop = false;
        var pastcount = storeCount;
        storeCount = new LinkedHashMap<>();
        for (BasicBlock block : function.blocks) {
             hasLoop= analyzeForBlock(block, function)||hasLoop;
        }
        if(hasLoop){
            var allkeys = new ArrayList<>(storeCount.keySet());
            allkeys.forEach(s->storeCount.put(s, 1));
        }
        storeCount.putAll(pastcount);
        for (BasicBlock a : function.blocks) {
            var ainfo = blockArrayInfos.get(a);
            for (var b : a.accessible) {
//                if(b.equals(a))
//                    continue;
                //遍历a可达的b
                var binfo = blockArrayInfos.get(b);
                //遍历a的memout
                for (var ptr : ainfo.memout.keySet()) {
//                    boolean funcPtrConflict = false;
                    //遍历b的funcin，如果有没有被覆盖的冲突，则该值对b无意义，跳过。这里想错了，就算被覆盖，也要在计数的时候用
//                    for (var funcptr : binfo.memFuncsin.keySet()) {
//                        if (ptrConflit(ptr, funcptr)) {
//                            for (var fromBlock : binfo.memFuncsin.get(funcptr)) {
//                                if (!mustPass(fromBlock, b, a)) {
//                                    funcPtrConflict = true;
//                                    break;
//                                }
//                            }
//                        }
//                        if (funcPtrConflict) {
//                            break;
//                        }
//                    }
//                    if (funcPtrConflict) {
//                        continue;
//                    }
                    //如果a的对应ptr的s中有phi，当前算法无法分析其效果，跳过分析，直接加入binfo的memin
                    Store s0 = ainfo.memout.get(ptr);
                    boolean canAdd = true;
                    boolean havePhi = false;
                    for(var ope : ptr.getOperands()){
                        if(ope instanceof Phi){
                            havePhi = true;
                            break;
                        }
                    }
                    if(!havePhi){
                        if (binfo.memin.containsKey(ptr)) {
                            ArrayList<Store> needDelete = new ArrayList<Store>();
                            for (var s : binfo.memin.get(ptr)) {
                                var fromBlock = s.getParent();
                                if(a.equals(b) || fromBlock.equals(b))continue;
                                if(fromBlock.equals(a)){
                                    //s和s0来源于同一个块，考察它们的先后关系
                                    if(s0.getParent().getInsts().indexOf(s0)>s.getParent().getInsts().indexOf(s)){
                                        needDelete.add(s);
                                        continue;
                                    }else{
                                        canAdd = false;
                                        break;
                                    }
                                }
                                //三个均不等
                                if (mustPass(fromBlock, b, a)) {
                                    needDelete.add(s);
                                } else if (mustPass(a, b, fromBlock)) {
                                    canAdd = false;
                                    break;
                                }
                            }
                            needDelete.forEach(binfo.memin.get(ptr)::remove);
                        }
                    }
                    if (canAdd) {
                        if (!binfo.memin.containsKey(ptr)) {
                            binfo.memin.put(ptr, new LinkedHashSet<>());
                        }
                        binfo.memin.get(ptr).add(s0);
                    }
                }
                //遍历a的funcsout
                for (var funcptr : ainfo.memFuncsout) {
                    //遍历b的memin，删掉所有覆盖值
//                    for (var ptr : binfo.memin.keySet()) {
//                        if (ptrConflit(ptr, funcptr)) {
//                            ArrayList<Store> needDelete = new ArrayList<>();
//                            for (var s : binfo.memin.get(ptr)) {
//                                BasicBlock mid = s.getParent();
//                                if (!mustPass(a, b, mid)) {
//                                    needDelete.add(s);
//                                }
//                            }
//                            needDelete.forEach(binfo.memin.get(ptr)::remove);
//                        }
//                    }
                    if (!binfo.memFuncsin.containsKey(funcptr)) {
                        binfo.memFuncsin.put(funcptr, new LinkedHashSet<>());
                    }
                    binfo.memFuncsin.get(funcptr).add(a);
                }
            }
        }
        //现在ArrayInfo中保留了所有可能产生影响的值
        for (BasicBlock b : function.blocks) {
            LinkedHashSet<Store> visitedStores = new LinkedHashSet<>();
            //遍历每一个块
            var info = blockArrayInfos.get(b);
            //遍历其中的mems
            for (Instr instr : info.mems) {
                if (instr instanceof Load) {
                    LinkedHashSet<Store> conflictStores = new LinkedHashSet<>();
                    GetElementPtrInst ptr = (GetElementPtrInst) instr.getOP(0);
                    info.memin.keySet().forEach(m ->
                    {
                        if (mayConflict(m, ptr)) {
                            conflictStores.addAll(info.memin.get(m));
                        }
                    });
                    boolean conflictWithFunc = false;
                    for (var funcptr : info.memFuncsin.keySet()) {
                        if (ptrConflit((GetElementPtrInst) ((Load) instr).getTarget(), funcptr)) {
                            conflictWithFunc = true;
                            break;
                        }
                    }
                    if (conflictWithFunc) {
                        conflictStores.forEach(s -> storeCount.put(s, storeCount.get(s) + 1));
                        continue;
                    }
                    if (info.memin.containsKey(ptr) && !info.memin.get(ptr).isEmpty()) {
                        if (info.memin.get(ptr).size() == 1) {

                            Store s0 = info.memin.get(ptr).iterator().next();
                            BasicBlock mid = s0.getParent();
                            if ((!mid.getDominate().contains(b)) || (mid.equals(b) &&
                                !visitedStores.contains(s0))) {
                                conflictStores.forEach(s -> storeCount.put(s, storeCount.get(s) + 1));
                                continue;
                            }

                            boolean canreplace = true;
                                //保证了mid不会等于b
                                for (var s : conflictStores) {
                                    if(visitedStores.contains(s0)){
                                        if(!visitedStores.contains(s)){
                                            continue;
                                        }
                                    }
                                    BasicBlock fromBlock = s.getParent();
                                    if (!mid.equals(fromBlock)) {
                                        if (!mustPass(fromBlock, b, mid)) {
                                            canreplace = false;
                                            break;
                                        }
                                    } else {
                                        if (mid.getInsts().indexOf(s0) <
                                            mid.getInsts().indexOf(s)) {
                                            canreplace = false;
                                            break;
                                        }
                                    }
                                }
                            if (canreplace) {
                                instr.replaceAllUsers(s0.getVal());
                                needrepeat = true;
                                continue;
                            }
                        }
                        conflictStores.forEach(s -> storeCount.put(s, storeCount.get(s) + 1));
                    } else {
                        if (!conflictStores.isEmpty()) {
                            conflictStores.forEach(s -> storeCount.put(s, storeCount.get(s) + 1));
                        } else {
                            if (ptr.getOP(0) instanceof Alloca) {
                                instr.replaceAllUsers(instr.type instanceof IntType ? new ConstNumber(0):
                                    new ConstNumber(0.0));
                                needrepeat = true;
                            } else if (ptr.getOP(0) instanceof Global) {
                                if (!((Global) ptr.getOP(0)).isInit) {
                                    instr.replaceAllUsers(new ConstNumber(0));
                                    needrepeat = true;
                                } else {
                                    boolean allConst = true;
                                    for (var ope : ptr.getOperands()) {
                                        if (ope instanceof Global) {
                                            continue;
                                        }
                                        if (!(ope instanceof ConstNumber)) {
                                            allConst = false;
                                            break;
                                            // TODO: 2023/8/15 可以做加强的优化，比如判断对应维度的值是否唯一。
                                        }
                                    }
                                    if (allConst) {
                                        ArrayList<Integer> pos = new ArrayList<Integer>();
                                        for (var ope : ptr.getOperands()) {
                                            if (ope instanceof ConstNumber) {
                                                pos.add((Integer) ((ConstNumber) ope).value);
                                            }
                                        }
                                        pos.remove(0);
                                        instr.replaceAllUsers(
                                            ((Global) ptr.getOP(0)).getValue(pos));
                                        needrepeat = true;
                                    }
                                }
                            }
                        }
                    }
                } else if (instr instanceof Store s0) {
                    visitedStores.add(s0);
                    //如果是重复ptr，覆盖所有原值，直接替换。
                    GetElementPtrInst ptr = (GetElementPtrInst) s0.getDest();
                    boolean havePhi = false;
                    for(var ope : ptr.getOperands()){
                        if(ope instanceof Phi){
                            havePhi = true;
                            break;
                        }
                    }
                    if(!havePhi){
                        if (info.memin.containsKey(ptr)) {
                            info.memin.get(ptr).clear();
                        }
                        if (!info.memin.containsKey(ptr)) {
                            info.memin.put(ptr, new LinkedHashSet<>());
                        }
                    }
                    info.memin.get(ptr).add(s0);
                } else if (instr instanceof Call) {
                    if (!(((Call) instr).getFunction().name.equals("memset") ||
                        ((Call) instr).getFunction().name.equals("putarray") ||
                        ((Call) instr).getFunction().name.equals("putfarray"))) {
                        //如果是call，加入memFuncsin即可，需要在Load时检查是否被覆盖，再做出替换决定。
                        //同时，涉及到的有冲突的store认为被用到了
                        for (var ope : ((Call) instr).getOperands()) {
                            if (ope instanceof GetElementPtrInst || ope instanceof Arg) {
                                if (!info.memFuncsin.containsKey(ope)) {
                                    info.memFuncsin.put( ope, new LinkedHashSet<>());
                                }
                                info.memFuncsin.get(ope).add(b);
                            }
                        }
                        if(((Call) instr).getFunction().name.equals(function.name)){
                            info.memFuncsin.put(new ConstNumber(0), new LinkedHashSet<>());
                        }
                    }
                    for(var ope : instr.getOperands()){
                        if(ope instanceof GetElementPtrInst || ope instanceof Arg){
                            for(var key : info.memin.keySet()){
                                if(ptrConflit(key, ope)){
                                    info.memin.get(key).forEach(e->storeCount.put(e, storeCount.get(e)+1));
                                }
                            }
                        }
                    }
                }
            }
        }
    }


}
