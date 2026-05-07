package midend;

import frontend.ir.Value;
import frontend.ir.cloner.LoopCloner;
import frontend.ir.constant.IRConst;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.AddInstr;
import frontend.ir.instr.otherop.AtomaticAddInstr;
import frontend.ir.instr.binop.SubInstr;
import frontend.ir.instr.convop.Bitcast;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.otherop.CmpInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.terminator.BranchInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.instr.terminator.ReturnInstr;
import frontend.ir.objecttype.TVoid;
import frontend.ir.objecttype.arithmetic.TFloat;
import frontend.ir.objecttype.arithmetic.TInt;
import frontend.ir.objecttype.derived.TArray;
import frontend.ir.objecttype.derived.TPointer;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.structure.IRModel;
import frontend.ir.symbol.ArrayInitVal;
import frontend.ir.symbol.GlbObjPtr;
import frontend.ir.symbol.Symbol;
import midend.Analysis.LoopInfo;
import midend.Analysis.SCEV.Expr.SCEVAddRecExpr;
import midend.Analysis.SCEV.Expr.SCEVConstant;
import midend.Analysis.SCEV.Expr.SCEVExpr;
import midend.Analysis.SCEV.SCEVAnalyse;
import util.Pair;

import java.util.*;

public class LoopParallel {
    private static IRModel model;
    private static HashMap<Value, SCEVExpr> scevInfo = new HashMap<>();
    private static final HashMap<PhiInstr, ArrayList<Pair<Instruction, Value>>> recMap = new HashMap<>();
    private static int loopBodyId = -1;
    private static final LinkedHashMap<Value, Integer> payLoad = new LinkedHashMap<>();
    private static final HashSet<Value> InPayLoad = new HashSet<>();
    private static final HashSet<Value> OutPayLoad = new HashSet<>();
    private static Function LoopParallelFor = null;
    private static int totalSize = 0;
    private static int init;

    private static CmpInstr indvar_cmp;
    private static Instruction indvar;

    public static void run(IRModel mod) {
        model = mod;
        for (Function function : mod.getFunctions()) {
            // 只针对 main 函数中的循环进行并行化处理
            if (!function.getName().equals("main")) continue;
            runOnFunc(function);
            break;
        }
    }

    public static void runOnFunc(Function function) {
        // 标量演化
        SCEVAnalyse.analyseFunc(function, false);
        // TODO: 将归纳变量终值变为常量
        scevInfo = function.getSCEVManager().getSCEVInfo();
        Set<LoopInfo> ancientLoopInfo = function.getAncientLoopInfo();
        ArrayList<LoopInfo> loops = new ArrayList<>(ancientLoopInfo);
        loops.sort(Comparator.comparingInt(a -> a.getLoopHeader().getDomLV()));
        for (LoopInfo loop : loops) {
            tryParallelLoop(loop);
        }
    }

    private static void tryParallelLoop(LoopInfo loop) {
        // 外层循环展不开就展内层，直到展开到可展的最外层为止
        if (!canParallelize(loop)) {
            for (LoopInfo child : new ArrayList<>(loop.getChildLoop())) {
                tryParallelLoop(child);
            }
            return;
        }

        // loopFunc为提出的循环函数
        loopBodyId++;
        Function loopFunc = new Function(new TVoid(), "parallel_body_" + loopBodyId);
        model.addFunctionToHead(loopFunc);
        loopFunc.setIsParallelLoopBody(true);

        /*
         * 1. payLoad: 传递的变量 -> 偏移量
         * 2. InPayLoad: 循环外定义，循环内使用的变量
         * 3. OutPayLoad: 循环内定义，循环外使用的变量
         */
        payLoad.clear();
        InPayLoad.clear();
        OutPayLoad.clear();
        totalSize = 0;
        HashSet<BasicBlock> loopBlocks = new HashSet<>();
        loop.getAllBlocks(loopBlocks);
        ArrayList<BasicBlock> blocks = new ArrayList<>(loopBlocks);
        for (BasicBlock bb : blocks) {
            for (Instruction instr : bb.getInstrList()) {
                for (Value op : instr.getOperands()) {
                    if (op instanceof Instruction instruction && !blocks.contains(instruction.getParentBB())) {
                        insertPayLoad(op, InPayLoad);
                    }
                }
            }
        }
        for (Value v : recMap.keySet()) {
            insertPayLoad(v, OutPayLoad);
        }

        //构建payload全局变量
        String payloadName = "parallel_payload_" + loopBodyId;
        GlbObjPtr payloadVar = new GlbObjPtr(new TArray(new TInt(), totalSize / 4), payloadName);
        Symbol payloadSym = new Symbol(payloadName, false, true, payloadVar.getType(), new ArrayInitVal.ArrayZeroInitVal(new TArray(new TInt(), totalSize / 4)), payloadVar);
        model.addGlobalValue(payloadSym);

        //给loopFunc添加参数，需要循环变量的初始值和循环变量的比较值
        List<Function.FParam> params = new ArrayList<>();
        Function.FParam beg = new Function.FParam(new TInt(), loopFunc.getAndAddRegIdx());
        Function.FParam end = new Function.FParam(new TInt(), loopFunc.getAndAddRegIdx());
        params.add(beg);
        params.add(end);
        loopFunc.setFParams(params);

        //构建entry块，这个entry块将作为新的preHeader
        BasicBlock entryBlock = new BasicBlock(loopFunc.getAndAddBlkIdx());
        loopFunc.appendBasicBlock(entryBlock);

        //克隆循环，获取循环块表与克隆前后Value映射表
        LoopCloner.getInstance().cloneLoopIntoFunc(loop, loopFunc);
        HashMap<Value, Value> reflectMap = loopFunc.getCloneInfo();

        //准备删除原有preHeader，相当于以构建的entryBlock为preHeader
        BasicBlock preHeaderToBeRemoved = (BasicBlock) loopFunc.getFirstBlk().getNext();

        BasicBlock exitBlock = new BasicBlock(loopFunc.getAndAddBlkIdx());
        loopFunc.appendBasicBlock(exitBlock);

        //将loop的各值同步到loopFunc中
        BasicBlock preHeader = loop.getPreHeader();
        BasicBlock nowLoopHeader = preHeaderToBeRemoved.getSucs().getFirst();
        PhiInstr newIndvar = (PhiInstr) reflectMap.get(indvar);
        Value oldPhiValue = newIndvar.getOperandMap().get(preHeaderToBeRemoved);
        newIndvar.conditionalModifyValue(preHeaderToBeRemoved, oldPhiValue, beg);
        preHeaderToBeRemoved.replaceUseWith(entryBlock);
        CmpInstr newIndvarCmp = (CmpInstr) reflectMap.get(indvar_cmp);
        Value oldCmpValue = newIndvarCmp.getOperand2();
        newIndvarCmp.modifyUse(oldCmpValue, end);
        oldCmpValue.getUserList().remove(newIndvarCmp);
        end.getUserList().add(newIndvarCmp);
        BranchInstr br = (BranchInstr) nowLoopHeader.getLastInstr();
        br.modifyUse(loop.getExitTarget(), exitBlock);
        loop.getExitTarget().getUserList().remove(br);
        exitBlock.getUserList().add(br);

        //把load-store对改成原子加
        HashSet<Instruction> toBeRemoved = new HashSet<>();
        for (Value rec : recMap.keySet()) {
            for (Pair<Instruction, Value> pair : recMap.get(rec)) {
                Instruction instr = pair.getKey();
                Value inc = pair.getValue();
                Instruction newInstr = (Instruction) reflectMap.get(instr);
                Value newInc = reflectMap.get(inc);
                BasicBlock instrBlock = newInstr.getParentBB();
                ArrayList<Value> offsets = new ArrayList<>();
                offsets.add(new IntConst(0));
                offsets.add(new IntConst(payLoad.get(rec) / 4));
                GEPInstr gep = new GEPInstr(loopFunc.getAndAddRegIdx(), offsets, payloadVar, instrBlock);
                gep.pureRemoveFromList();
                instrBlock.insertInsBefore(gep, newInstr);
                AtomaticAddInstr atomicAddInstr = new AtomaticAddInstr(gep, newInc, instrBlock);
                atomicAddInstr.pureRemoveFromList();
                instrBlock.insertInsBefore(atomicAddInstr, newInstr);
                toBeRemoved.add(newInstr);
            }
        }

        //清除掉原子加对应变量的phi
        for (PhiInstr phi : nowLoopHeader.getPhiInstrs()) {
            for (Value op : phi.getOperands()) {
                if (toBeRemoved.contains(op)) {
                    phi.forceRemoveFromList();
                    break;
                }
            }
        }
        for (Instruction instr : toBeRemoved) {
            instr.forceRemoveFromList();
        }

        //加入调用并行函数的块
        Function oriFunc = loop.getLoopHeader().getParentFunc();
        BasicBlock callBlock = new BasicBlock(oriFunc.getAndAddBlkIdx());
        oriFunc.appendBasicBlock(callBlock);
        if (loop.getParentLoop() != null) {
            callBlock.setLoopBelonged(loop.getParentLoop());
            loop.getParentLoop().addBodyBlock(callBlock);
        }
        preHeader.getLastInstr().forceRemoveFromList();
        new JumpInstr(callBlock, preHeader);

        //添加存取InPayLoad的指令
        for (Value value : InPayLoad) {
            //在callBlock中存
            ArrayList<Value> offsets = new ArrayList<>();
            offsets.add(new IntConst(0));
            offsets.add(new IntConst(payLoad.get(value) / 4));
            Value gep = new GEPInstr(oriFunc.getAndAddRegIdx(), offsets, payloadVar, callBlock);
            if (value.getType() instanceof TFloat || value.getType() instanceof TPointer) {
                gep = new Bitcast(oriFunc.getAndAddRegIdx(), gep, new TPointer(value.getType()), callBlock);
            }
            new StoreInstr(value, gep, callBlock);
            //在loopFunc中取
            value = reflectMap.getOrDefault(value, value);
            Value gep1 = new GEPInstr(loopFunc.getAndAddRegIdx(), offsets, payloadVar, entryBlock);
            if (value.getType() instanceof TFloat || value.getType() instanceof TPointer) {
                gep1 = new Bitcast(loopFunc.getAndAddRegIdx(), gep1, new TPointer(value.getType()), entryBlock);
            }
            LoadInstr load = new LoadInstr(loopFunc.getAndAddRegIdx(), value.getType(), gep1, entryBlock);
            ArrayList<Instruction> replaceUsers = new ArrayList<>();
            for (Value user : value.getUserList()) {
                Instruction instr = (Instruction) user;
                if (instr.getParentBB().getParentFunc() == null
                        || !instr.getParentBB().getParentFunc().equals(loopFunc)) {
                    continue;
                }
                replaceUsers.add(instr);
            }
            for (Instruction instr : replaceUsers) {
                instr.modifyUse(value, load);
                value.getUserList().remove(instr);
                load.getUserList().add(instr);
            }
        }
        new JumpInstr(nowLoopHeader, entryBlock);

        //添加存取OutPayLoad的指令
        for (Value outValue : OutPayLoad) {
            //赋上初始值
            Value val = ((PhiInstr) outValue).getOperandMap().get((loop.getLoopHeader()));
            if (val instanceof PhiInstr phiInstr) {
                Value init = phiInstr.getOperandMap().get(loop.getPreHeader());
                ArrayList<Value> offsets = new ArrayList<>();
                offsets.add(new IntConst(0));
                offsets.add(new IntConst(payLoad.get(outValue) / 4));
                GEPInstr gep = new GEPInstr(loopFunc.getAndAddRegIdx(), offsets, payloadVar, callBlock);
                new StoreInstr(init, gep, callBlock);
            }
        }

        //构造call指令
        ArrayList<Value> args = new ArrayList<>();
        args.add(new IntConst(init));
        args.add(indvar_cmp.getOperand2());
        args.add(loopFunc);
        model.setHasParallel(true);
        new CallInstr(null, getLoopParallelFuncLib(), args, callBlock);

        //把循环函数修改的值load回来
        for (Value outValue : OutPayLoad) {
            ArrayList<Value> offsets = new ArrayList<>();
            offsets.add(new IntConst(0));
            offsets.add(new IntConst(payLoad.get(outValue) / 4));
            GEPInstr gep = new GEPInstr(loopFunc.getAndAddRegIdx(), offsets, payloadVar, callBlock);
            LoadInstr load = new LoadInstr(loopFunc.getAndAddRegIdx(), outValue.getType(), gep, callBlock);

            //替换掉退出块的LCSSA
            PhiInstr lcssaPhi = (PhiInstr) outValue;
            Value recValue = lcssaPhi.getOperandMap().get(loop.getLoopHeader());
            lcssaPhi.conditionalModifyValue(loop.getLoopHeader(), recValue, load);
//            lcssaPhi.changePreBlock(loop.getLoopHeader(), callBlock);
            loop.getLoopHeader().replaceUseWith(callBlock);
        }
        new JumpInstr(loop.getExitTarget(), callBlock);
        new ReturnInstr(null, exitBlock);
        preHeaderToBeRemoved.removeFromListClear();

        //删除原有循环信息，避免重复循环展开
        if (loop.getParentLoop() == null) {
            oriFunc.getAncientLoopInfo().remove(loop);
        } else {
            loop.getParentLoop().getChildLoop().remove(loop);
        }
        for (BasicBlock bb : loop.getBodyBlocks()) {
            bb.removeFromList();
        }
    }

    private static void insertPayLoad(Value value, HashSet<Value> payLoadSet) {
        if (value instanceof IRConst) return;
        if (value.equals(indvar)) return; // 循环变量不需要放入 payload
        if (payLoadSet.contains(value)) return; // 已经在 payload 中
        int size = value.getType().getByteSize();
        if ((totalSize & 15) != 0) {
            totalSize += (16 - (totalSize & 15)) & 15;
        }
        payLoad.put(value, totalSize);
        totalSize += size;
        payLoadSet.add(value);
    }

    private static boolean canParallelize(LoopInfo loop) {
        if (loop.getLoopCondition() == null) return false;
        SCEVExpr tripCountExpr = loop.getLoopCondition().getTripCountExpr();
        if (tripCountExpr instanceof SCEVConstant constant && constant.value < 16) {
            return false;
        }
        if (loop.getExitTargets().size() != 1) return false;
        List<BasicBlock> pres = loop.getExitTarget().getPres();
        if (pres.size() > 1 || pres.getFirst() != loop.getLoopHeader()) {
            return false;
        }
        Instruction terminator = loop.getLoopHeader().getLastInstr();
        if (!(terminator instanceof BranchInstr br)) return false;
        if (!(br.getCondition() instanceof CmpInstr cmpInstr && !cmpInstr.getCond().isForFloat())) {
            return false;
        }
        if (cmpInstr.getCond() == CmpInstr.CmpCond.ILE
                && cmpInstr.getOperand2() instanceof PhiInstr
                && ((PhiInstr) cmpInstr.getOperand2()).getOperandMap().size() == 1) {
            return false;
        }
        // TODO: 为什么不能翻转，困惑
//        if (scevInfo.get(cmpInstr.getOperand2()) instanceof SCEVAddRecExpr) {
//            cmpInstr.swap();
//        }
        if (scevInfo.get(cmpInstr.getOperand2()) instanceof SCEVAddRecExpr) {
            return false;
        }
        if (!scevInfo.containsKey(cmpInstr.getOperand1())
                || scevInfo.get(cmpInstr.getOperand1()) == null) {
            return false;
        }
        if (!((cmpInstr.getOperand1()) instanceof PhiInstr phiInstr)) {
            return false;
        }
        indvar_cmp = cmpInstr;
        indvar = phiInstr;
        SCEVExpr scev = scevInfo.get(cmpInstr.getOperand1());
        if (scev instanceof SCEVAddRecExpr addRecExpr && !addRecExpr.inTheSameLoop()) {
            return false;
        }
        init = scev instanceof SCEVConstant constant ? constant.value :
                scev instanceof SCEVAddRecExpr addRec ? addRec.operands.getFirst() instanceof SCEVConstant c ? c.value : 0 : 0;
        int step = scev instanceof SCEVConstant ? 0 :
                scev instanceof SCEVAddRecExpr addRec ? addRec.operands.get(1) instanceof SCEVConstant c ? c.value : 0 : 0;
        if (step != 1) return false;
        recMap.clear();
        for (PhiInstr phi : loop.getExitTarget().getPhiInstrs()) {
            if (!phi.isLCSSA()) continue;
            recMap.put(phi, new ArrayList<>());
            Value val = phi.getOperandMap().get(loop.getLoopHeader());
            if (!(val instanceof PhiInstr recPhi)) return false;
            if (recPhi.getUserList().size() != 2) return false;
            Value recVal = recPhi.getOperandMap().get(loop.getOnlyLatch());
            if (recVal instanceof AddInstr addInstr) {
                if (!addInstr.getOperand1().equals(recPhi) && !addInstr.getOperand2().equals(recPhi)) {
                    return false;
                }
                Value inc = addInstr.getOperand1().equals(recPhi) ? addInstr.getOperand2() : addInstr.getOperand1();
                recMap.get(phi).add(new Pair<>(addInstr, inc));
            } else if (recVal instanceof SubInstr subInstr) {
                if (!subInstr.getOperand1().equals(recPhi)) {
                    return false;
                }
                Value dec = subInstr.getOperand2();
                recMap.get(phi).add(new Pair<>(subInstr, dec));
            }
//            } else if (recVal instanceof PhiInstr recPhi2) {
//                // TODO: 嵌套循环的情况
//                if (!recPhi2.isLCSSA()) {
//                    return false;
//                }
//                Value subVal = recPhi2.getOperandMap().get(recPhi2.getParentBB().getPres().getFirst());
//                if (!(subVal instanceof PhiInstr subPhiInstr)) {
//                    return false;
//                }
//
//            }
            else {
                return false;
            }

        }
        // 循环独立性检查
        HashMap<Value, Integer> loadStoreMap = new HashMap<>();
        HashSet<BasicBlock> loopBlocks = new HashSet<>();
        loop.getAllBlocks(loopBlocks);
        for (BasicBlock block : loopBlocks) {
            for (Instruction instr : block.getInstrList()) {
                if (instr instanceof LoadInstr loadInstr) {
                    Value ptr = loadInstr.getPointer();
                    Value baseAddr = getBaseAddr(ptr);
                    loadStoreMap.put(baseAddr, loadStoreMap.getOrDefault(baseAddr, 0) | 1);
                } else if (instr instanceof StoreInstr storeInstr) {
                    Value ptr = storeInstr.getPointer();
                    Value baseAddr = getBaseAddr(ptr);
                    loadStoreMap.put(baseAddr, loadStoreMap.getOrDefault(baseAddr, 0) | 2);
                } else if (instr instanceof CallInstr) {
                    return false; // 函数调用不支持并行化
                } else if (instr instanceof AtomaticAddInstr) {
                    return false; // 原子加法不支持并行化
                }
            }
        }

        //原子指令转化
        for (Value key : loadStoreMap.keySet()) {
            int flag = loadStoreMap.get(key);
            if (flag == 3) {
                LoadInstr load = null;
                for (BasicBlock block : loopBlocks) {
                    for (Instruction instr : block.getInstrList()) {
                        if (instr instanceof LoadInstr loadInstr) {
                            if (getBaseAddr(loadInstr.getPointer()).equals(key)) {
                                if (load != null) return false;
                                load = loadInstr;
                            }
                        } else if (instr instanceof StoreInstr storeInstr) {
                            if (getBaseAddr(storeInstr.getPointer()).equals(key)) {
                                if (load == null) return false;
                                if (load.getUserList().size() == 1
                                        && load.getPointer() == storeInstr.getPointer()
                                        && load.getParentBB() == storeInstr.getParentBB()) {
                                    Value storeVal = storeInstr.getValueToStore();
                                    if (!(storeVal instanceof AddInstr addInstr)) return false;
                                    if (addInstr.getOperands().contains(load) &&
                                            addInstr.getUserList().size() == 1) {
                                        load = null;
                                    } else {
                                        return false;
                                    }
                                } else {
                                    return false;
                                }
                            }
                        }
                    }
                }
                if (load != null) return false; // 有 Load 没有 Store
            } else if (flag == 2) {
                // TODO: 对同时写的处理
            }
        }
        //recMap中的减法换成加法
        for (PhiInstr rec : recMap.keySet()) {
            ArrayList<Pair<Instruction, Value>> recList = recMap.get(rec);
            recMap.put(rec, new ArrayList<>());
            for (Pair<Instruction, Value> pair : recList) {
                Instruction instr = pair.getKey();
                Value inc = pair.getValue();
                if (instr instanceof AddInstr) {
                    recMap.get(rec).add(pair);
                    continue;
                }
                SubInstr subInstr = (SubInstr) instr;
                BasicBlock block = subInstr.getParentBB();
                SubInstr newSub = new SubInstr(block.getParentFunc().getAndAddRegIdx(), new IntConst(0), inc, block);
                newSub.removeFromList();
                block.insertInsBefore(newSub, subInstr);
                AddInstr addInstr = new AddInstr(block.getParentFunc().getAndAddRegIdx(), subInstr.getOperand1(), newSub, block);
                addInstr.removeFromList();
                block.insertInsBefore(addInstr, subInstr);
                subInstr.replaceUseWith(addInstr);
                subInstr.removeFromList();
                recMap.get(rec).add(new Pair<>(addInstr, newSub));
            }
        }
        return true;
    }

    private static Value getBaseAddr(Value instr) {
        Value ret = instr;
        while (ret instanceof GEPInstr || ret instanceof Bitcast) {
            if (ret instanceof GEPInstr gep) {
                ret = gep.getPtrVal();
            } else {
                Bitcast bitcast = (Bitcast) ret;
                ret = bitcast.getValue();
            }
        }
        return ret;
    }

    private static Function getLoopParallelFuncLib() {
        if (LoopParallelFor != null) return LoopParallelFor;
        LoopParallelFor = new Function(new TVoid(), "OHMParallelFor");
        return LoopParallelFor;
    }
}