package midend;

import frontend.ir.Value;
import frontend.ir.constant.IRConst;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.MulInstr;
import frontend.ir.instr.convop.Bitcast;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.Analysis.LoopContentAnalyse;
import midend.Analysis.LoopInfo;
import midend.Analysis.SCEV.Expr.SCEVExpr;
import midend.Analysis.SCEV.LoopCondition;

import java.util.*;

public class Loop2Memset {
    public static void execute(List<Function> functions) {
        for (Function function : functions) {
            Set<LoopInfo> ancientLoopInfo = new HashSet<>(function.getAncientLoopInfo());
            for (LoopInfo loopInfo : ancientLoopInfo) {
                dfsLoop(loopInfo);
            }
        }
    }

    private static boolean dfsLoop(LoopInfo loopInfo) { // 成功转化本层循环为 memset 则返回 true
        Set<LoopInfo> childLoops = new HashSet<>(loopInfo.getChildLoop());
        int remainingCnt = childLoops.size();
        if (remainingCnt > 0) {
            for (LoopInfo childLoop : childLoops) {
                if (dfsLoop(childLoop)) {
                    remainingCnt -= 1;
                }
            }
        } else {
            LoopContentAnalyse.analyseLoop(loopInfo);
            if (loopInfo.loopContents == null || loopInfo.loopContents.isEmpty()) {
                return false;
            }
            LoopCondition loopCond = loopInfo.loopCondition;
            if (loopCond == null || loopInfo.getLoopVarStepConst() == null || loopInfo.getLoopVarStepConst().getConstInt() != 1) {
                return false;
            }

            Map<StoreInstr, GEPInstr> store2ptr = new HashMap<>();
            Map<GEPInstr, IntConst> ptr2content = new HashMap<>();
            List<GEPInstr> gepList = new LinkedList<>();
            for (int i = loopInfo.loopContents.size() - 1; i >= 0; i--) {
                Instruction ins = loopInfo.loopContents.get(i);
                if (ins instanceof StoreInstr storeInstr) {
                    if (storeInstr.getValueToStore() instanceof IRConst irConst
                            && storeInstr.getPointer() instanceof GEPInstr gepInstr) {
                        store2ptr.put(storeInstr, gepInstr);
                        if (irConst.getConstVal().intValue() == 0) {
                            ptr2content.put(gepInstr, new IntConst(0));
                        } else if (irConst.getConstVal().intValue() == -1) {
                            ptr2content.put(gepInstr, new IntConst(-1));
                        } else {
                            return false;
                        }
                    } else {
                        return false;
                    }
                } else if (ins instanceof GEPInstr gepInstr) {
                    if (store2ptr.containsValue(gepInstr) && gepInstr.getUserList().size() == 1) {
                        gepList.addFirst(gepInstr);
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
            }

            if (!(new HashSet<>(gepList).containsAll(store2ptr.values()))) {
                throw new RuntimeException("循环转 memset 遇到了未曾设想的情况：所有 GEP 都应该随循环迭代");
            }

            BasicBlock preHeader = loopInfo.getPreHeader();
            Instruction lastIns = preHeader.getLastInstr();
            Value loopVar = loopCond.loopVar;
            IntConst beginning = loopInfo.getLoopVarBeginningConst();
            Function parentFunc = loopInfo.getFunction();

            SCEVExpr tripCountExpr = loopCond.getTripCountExpr();
            if (tripCountExpr == null) {
                return false;
            }
            SCEVExpr tripCountSimplified = tripCountExpr.simplify();
            loopCond.tripCountExpr = tripCountSimplified;
            if (tripCountSimplified.getValueProjected() == null) {
                tripCountSimplified.instantiation(preHeader);
            }
            final Value tripCount = tripCountSimplified.getValueProjected();

            if (loopVar == null || beginning == null || tripCount == null) {
                return false;
            }
            if (loopVar.getUserList().size() != store2ptr.size() + 2) { // 简单的循环，循环变量除了用来 gep 之外只有用来累加和判断结束条件两个使用
                return false;
            }

            for (StoreInstr storeInstr : store2ptr.keySet()) {
                storeInstr.removeFromList();
                store2ptr.get(storeInstr).removeFromList();
            }

            for (GEPInstr gepInstr : gepList) {
                Value ptrVal = gepInstr.getPtrVal();
                List<Value> indexList = new ArrayList<>(gepInstr.getIndexList());
                indexList.replaceAll(index -> index == loopVar ? beginning : index);
                GEPInstr newGEP = new GEPInstr(
                        parentFunc.getAndAddRegIdx(), indexList, ptrVal, preHeader
                );
                newGEP.setUse(ptrVal);
                for (Value index : indexList) {
                    newGEP.setUse(index);
                }
                newGEP.insertBefore(lastIns);
                Value lastIndex = indexList.getLast();
                if (lastIndex instanceof Instruction insIdx && insIdx != loopVar) {
                    insIdx.pureRemoveFromList();
                    insIdx.insertBefore(newGEP);
                }
                loopVar.replaceUseWith(beginning);

                Bitcast toI8 = new Bitcast(parentFunc.getAndAddRegIdx(), newGEP, preHeader);
                toI8.setUse(newGEP);
                toI8.insertBefore(lastIns);

                IntConst elementSize = new IntConst(4);
                MulInstr mulInstr = new MulInstr(parentFunc.getAndAddRegIdx(), tripCount, elementSize, preHeader);
                mulInstr.setUse(tripCount);
                mulInstr.setUse(elementSize);
                mulInstr.insertBefore(loopInfo.getAncientLoop().getPreHeader().getLastInstr());     // 先扔到循环之外，免得影响后续迭代（具体顺序正确性由后续 GCM 保证）

                List<Value> rParamList = Arrays.asList(toI8, ptr2content.get(gepInstr), mulInstr);
                CallInstr memset = new CallInstr(null, Function.LibFunc.MEMSET, rParamList, preHeader);
                Function.LibFunc.MEMSET.addCallCnt();
                loopInfo.getFunction().addCallee(Function.LibFunc.MEMSET);
                for (Value rParam : rParamList) {
                    memset.setUse(rParam);
                }
                memset.insertBefore(lastIns);
            }

            loopInfo.deleteCurrentLoop();
            return true;
        }

        // 外层不能说展开就展开，因为 memset 不一定要清空整个数组
//        if (remainingCnt == 0) {
//            LoopContentAnalyse.analyseLoop(loopInfo);
//            if (loopInfo.loopContents == null || loopInfo.loopContents.isEmpty()) {
//                return false;
//            }
//            LoopCondition loopCond = loopInfo.loopCondition;
//            if (loopCond == null || loopInfo.getLoopVarStepConst() == null || loopInfo.getLoopVarStepConst().getConstInt() != 1) {
//                return false;
//            }
//
//            Map<CallInstr, Bitcast> memset2cast = new HashMap<>();
//            Map<Bitcast, GEPInstr> cast2gep = new HashMap<>();
//            List<Bitcast> castList = new LinkedList<>();
//            List<GEPInstr> gepList = new LinkedList<>();
//            for (int i = loopInfo.loopContents.size() - 1; i >= 0; i--) {
//                Instruction ins = loopInfo.loopContents.get(i);
//                switch (ins) {
//                    case CallInstr callInstr when callInstr.getCallee() == Function.LibFunc.MEMSET -> {
//                        if (callInstr.getRParams().get(1) instanceof IRConst irConst
//                                && irConst.getConstVal().intValue() == 0
//                                && callInstr.getRParams().get(0) instanceof Bitcast bitcast) {
//                            memset2cast.put(callInstr, bitcast);
//                        } else {
//                            return false;
//                        }
//                    }
//                    case Bitcast bitcast -> {
//                        if (memset2cast.containsValue(bitcast) && bitcast.getUserList().size() == 1
//                                && bitcast.getValue() instanceof GEPInstr gepInstr) {
//                            castList.addFirst(bitcast);
//                            cast2gep.put(bitcast, gepInstr);
//                        } else {
//                            return false;
//                        }
//                    }
//                    case GEPInstr gepInstr -> {
//                        if (cast2gep.containsValue(gepInstr) && gepInstr.getUserList().size() == 1) {
//                            List<Value> indexList = gepInstr.getIndexList();
//                            int idx = indexList.indexOf(loopCond.loopVar) + 1;
//                            while (idx < indexList.size()) {
//                                if (!(indexList.get(idx) instanceof IntConst intConst && intConst.getConstInt() == 0)) {
//                                    return false;   // 本层迭代之下的所有层必须从 0 开始，否则算不了
//                                }
//                                idx++;
//                            }
//                            gepList.addFirst(gepInstr);
//                        } else {
//                            return false;
//                        }
//                    }
//                    case null, default -> {
//                        return false;
//                    }
//                }
//            }
//
//            if (!(new HashSet<>(castList).containsAll(memset2cast.values()))) {
//                throw new RuntimeException("循环转 memset 遇到了未曾设想的情况：所有 bitcast 都应该随循环迭代");
//            }
//            if (!(new HashSet<>(gepList).containsAll(cast2gep.values()))) {
//                throw new RuntimeException("循环转 memset 遇到了未曾设想的情况：所有 GEP 都应该随循环迭代");
//            }
//
//            for (CallInstr callInstr : memset2cast.keySet()) {
//                callInstr.removeFromList();
//            }
//            for (Bitcast bitcast : castList) {
//                bitcast.removeFromList();
//            }
//            for (GEPInstr gepInstr : gepList) {
//                gepInstr.removeFromList();
//            }
//
//            Map<GEPInstr, CallInstr> gep2memset = new HashMap<>();
//            for (CallInstr memset : memset2cast.keySet()) {
//                gep2memset.put(cast2gep.get(memset2cast.get(memset)), memset);
//            }
//
//            BasicBlock preHeader = loopInfo.getPreHeader();
//            Instruction lastIns = preHeader.getLastInstr();
//            Value loopVar = loopCond.loopVar;
//            Value loopBound = loopCond.bound;
//            Value beginning = loopInfo.getLoopVarBeginningConst();
//            Function parentFunc = loopInfo.getFunction();
//            for (GEPInstr gepInstr : gepList) {
//                Value ptrVal = gepInstr.getPtrVal();
//                List<Value> indexList = new ArrayList<>(gepInstr.getIndexList());
//                indexList.replaceAll(index -> index == loopVar ? beginning : index);
//                GEPInstr newGEP = new GEPInstr(
//                        parentFunc.getAndAddRegIdx(), indexList, ptrVal, preHeader
//                );
//                newGEP.setUse(ptrVal);
//                for (Value index : indexList) {
//                    newGEP.setUse(index);
//                }
//                newGEP.insertBefore(lastIns);
//
//                Bitcast toI8 = new Bitcast(parentFunc.getAndAddRegIdx(), newGEP, preHeader);
//                toI8.setUse(newGEP);
//                toI8.insertBefore(lastIns);
//
//                Value lowerBound = gep2memset.get(gepInstr).getRParams().getLast();
//                MulInstr mulInstr = new MulInstr(parentFunc.getAndAddRegIdx(), lowerBound, loopBound, preHeader);
//                mulInstr.setUse(lowerBound);
//                mulInstr.setUse(loopBound);
//                mulInstr.insertBefore(loopInfo.getAncientLoop().getPreHeader().getLastInstr());     // 先扔到循环之外，免得影响后续迭代（具体顺序正确性由后续 GCM 保证）
//
//                List<Value> rParamList = Arrays.asList(toI8, new IntConst(0), mulInstr);
//                CallInstr memset = new CallInstr(null, Function.LibFunc.MEMSET, rParamList, preHeader);
//                Function.LibFunc.MEMSET.addCallCnt();
//                loopInfo.getFunction().addCallee(Function.LibFunc.MEMSET);
//                for (Value rParam : rParamList) {
//                    memset.setUse(rParam);
//                }
//                memset.insertBefore(lastIns);
//            }
//
//            loopInfo.deleteCurrentLoop();
//            return true;
//        }

        return false;
    }
}
