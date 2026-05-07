package midend;

import frontend.ir.Value;
import frontend.ir.constant.IntConst;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.AddInstr;
import frontend.ir.instr.binop.MulInstr;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.objecttype.arithmetic.TFloat;
import frontend.ir.objecttype.derived.TPointer;
import frontend.ir.objecttype.derived.TVector;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import midend.Analysis.LoopInfo;
import midend.Analysis.SCEV.Expr.*;
import midend.Analysis.SCEV.LoopCondition;
import midend.pseudoinstr.*;

import java.util.*;

public class VectorizationDetect {
    private static final Set<BasicBlock> NO_VEC_BB_SET = new HashSet<>();

    public static void execute(List<Function> functions) {
        NO_VEC_BB_SET.clear();

        for (Function function : functions) {
            for (LoopInfo loopInfo : function.getAncientLoopInfo()) {
                dfsLoop(loopInfo, function);
            }
            for (BasicBlock basicBlock : function.getBasicBlockList()) {
                runOnBB(basicBlock);
            }
        }
    }

    private static void dfsLoop(LoopInfo loopInfo, Function function) {
        if (!loopInfo.getChildLoop().isEmpty()) {
            for (LoopInfo childLoopInfo : loopInfo.getChildLoop()) {
                dfsLoop(childLoopInfo, function);
            }
            return;
        }

        boolean goodBeginning = loopInfo.loopCondition instanceof LoopCondition loopCond
                && loopCond.loopVarExprSpecial instanceof SCEVVecAddRecExpr addRec
                && addRec.getOperands().getFirst() instanceof SCEVConstant constant && (constant.value & 3) == 0;

        if (!goodBeginning) {
            NO_VEC_BB_SET.addAll(loopInfo.getBodyBlocks());
            return;
        }

        Instruction instruction = loopInfo.getLoopHeader().getFirstInstr();

        List<SCEVVecAddRecExpr> scevVecAddRecExprList = new ArrayList<>();
        while (instruction instanceof PhiInstr phiInstr) {
            SCEVExpr scevExpr = function.getSCEVManager().getSCEV(phiInstr);
            if (scevExpr instanceof SCEVVecAddRecExpr) {
                scevVecAddRecExprList.add((SCEVVecAddRecExpr) scevExpr);
            }

            instruction = (Instruction) instruction.getNext();
        }
        if (scevVecAddRecExprList.size() != 2) {  // todo 能否处理多于 2 条的情况？以及有没有这种情况需要处理？
            return;
        }

        SCEVVecAddRecExpr cyclicVariable;           // 循环变量
        SCEVVecAddRecExpr accumulatedVariable;      // 累加变量

        if (isVecCyclicVariable(scevVecAddRecExprList.get(0))) {
            cyclicVariable = scevVecAddRecExprList.get(0);
            accumulatedVariable = scevVecAddRecExprList.get(1);
        } else if (isVecCyclicVariable(scevVecAddRecExprList.get(1))) {
            cyclicVariable = scevVecAddRecExprList.get(1);
            accumulatedVariable = scevVecAddRecExprList.get(0);
        } else {
            return;     // todo 能否处理除了 +1 之外的其它循环方式
        }

        // todo 暂时只处理只累加 load 的情况
        List<LoadInstr> loadList = new ArrayList<>();
        List<GEPInstr> ldPtrList = new ArrayList<>();
        for (int i = 1; i < accumulatedVariable.getOperands().size(); i++) {
            if (accumulatedVariable.getOperands().get(i) instanceof SCEVUnknown unknown) {
                if (unknown.val instanceof LoadInstr ld && ld.getPointer() instanceof GEPInstr ptr) {
                    loadList.add(ld);
                    ldPtrList.add(ptr);
                } else {
                    return;
                }
            } else {
                return;
            }
        }

        List<Value> itList = new ArrayList<>();
        itList.add(cyclicVariable.getValueProjected());
        itList.addAll(cyclicVariable.getAdds().subList(0, 3));
        if (!(loadList.size() == 4 && ldPtrList.size() == 4 && checkPtr(loadList.getFirst().getParentBB(), ldPtrList, itList))) {
            return;
        }

        // (add 1) * 4 -> add 4
        int regIdx1 = function.getAndAddRegIdx();
        BasicBlock curBB = cyclicVariable.getAdds().getLast().getParentBB();
        AddInstr newAdd = new AddInstr(
                regIdx1, cyclicVariable.getValueProjected(), new IntConst(4), curBB
        );
        newAdd.insertAfter(cyclicVariable.getAdds().getLast());
        cyclicVariable.getAdds().getLast().replaceUseWith(newAdd);

        // 把 phi 的 Type 换成 VEC
        int regIdx2 = function.getAndAddRegIdx();
        if (!(accumulatedVariable.getValueProjected() instanceof PhiInstr oldPhi)) {
            throw new RuntimeException("<UNK>");
        }
        PhiInstr newPhi = new PhiInstr(
                new TVector(oldPhi.getType()), regIdx2, oldPhi.getParentBB(), oldPhi.isLCSSA()
        );
        for (BasicBlock basicBlock : oldPhi.getOperandMap().keySet()) {
            newPhi.addOperand(basicBlock, oldPhi.getOperandMap().get(basicBlock));
        }
        newPhi.insertAfter(oldPhi);
        oldPhi.replaceUseWith(newPhi);

        BasicBlock curBB1 = ldPtrList.getFirst().getParentBB();
        int regIdx3 = function.getAndAddRegIdx();
        VecLoadInstr vLoadInstr = new VecLoadInstr(regIdx3, ldPtrList.getFirst(), curBB1);
        int regIdx4 = function.getAndAddRegIdx();
        VecAddInstr  vAddInstr  = new VecAddInstr(newPhi, vLoadInstr, regIdx4, curBB1);

        AddInstr finalAdd = accumulatedVariable.getAdds().getLast();
        vLoadInstr.insertAfter(finalAdd);
        vAddInstr.insertAfter(vLoadInstr);
        finalAdd.replaceUseWith(vAddInstr);

//        System.out.println(cyclicVariable);
//        System.out.println(accumulatedVariable);
//        System.out.println();
    }

    private static boolean isVecCyclicVariable(SCEVVecAddRecExpr scevVecAddRecExpr) {
        List<SCEVExpr> operands = scevVecAddRecExpr.getOperands();
        if (!(operands.getFirst() instanceof SCEVConstant constant && constant.value == 0)) {
            return false;
        }
        for (int i = 1; i < operands.size(); i++) {
            SCEVExpr operand = operands.get(i);
            if (!(operand instanceof SCEVConstant constantI && constantI.value == 1)) {
                return false;
            }
        }
        return true;
    }

    private static void runOnBB(BasicBlock basicBlock) {
        if (NO_VEC_BB_SET.contains(basicBlock)) {
            return;
        }

        VectorizableBlock vectorizableBlock = getVectorizableBlock(basicBlock);
        if (vectorizableBlock == null) {
            return;
        }
//        System.out.println("vectorizableBlock = " + vectorizableBlock.basicBlock.toString());

        mergeAdd(vectorizableBlock);
        if (detectMulAdd(vectorizableBlock)) {
            return;
        }
        if (vectorizableBlock.basicBlock.getInstrList().size() <= 64) {
            storeVectorization(vectorizableBlock);  // 对于简单的存储块可以向量化，太大的块存储量之间的计算本身就很远，变量活跃区间过长
        }
    }

    private static boolean detectMulAdd(VectorizableBlock vectorizableBlock) {
        List<Value> mulOp1List = new ArrayList<>();
        List<Value> mulOp2List = new ArrayList<>();
        List<GEPInstr> mulOp1PtrList = new ArrayList<>();
        List<GEPInstr> mulOp2PtrList = new ArrayList<>();

        List<StoreInstr> storeList = vectorizableBlock.storeList;
        List<GEPInstr> pointerList = vectorizableBlock.storePtrList;
        for (int i = 0; i < storeList.size(); i++) {
            StoreInstr storeInstr = storeList.get(i);
            if (storeInstr.getValueToStore() instanceof AddInstr addInstr) {
                Value anotherAddOp;
                if (addInstr.getOperand1() instanceof LoadInstr loadInstr && loadInstr.getPointer() == pointerList.get(i)) {
                    anotherAddOp = addInstr.getOperand2();
                } else if (addInstr.getOperand2() instanceof LoadInstr loadInstr && loadInstr.getPointer() == pointerList.get(i)) {
                    anotherAddOp = addInstr.getOperand1();
                } else {
                    return false;
                }

                // todo 这里可能可以做更多情况，现在只有都是 load 的情况
                if (anotherAddOp instanceof MulInstr mulInstr
                        && mulInstr.getOperand1() instanceof LoadInstr ld1 && ld1.getPointer() instanceof GEPInstr gep1
                        && mulInstr.getOperand2() instanceof LoadInstr ld2 && ld2.getPointer() instanceof GEPInstr gep2) {
                    mulOp1List.add(ld1);
                    mulOp2List.add(ld2);
                    mulOp1PtrList.add(gep1);
                    mulOp2PtrList.add(gep2);
                } else {
                    return false;
                }
            } else {
                return false;
            }
        }

        if (mulOp1PtrList.size() != 4 || mulOp2PtrList.size() != 4) {
            return false;
        }

        boolean isSamePtr1 = mulOp1PtrList.get(0) == mulOp1PtrList.get(1)
                && mulOp1PtrList.get(1) == mulOp1PtrList.get(2)
                && mulOp1PtrList.get(2) == mulOp1PtrList.get(3);
        boolean isCorrectPtr1 = checkPtr(vectorizableBlock.basicBlock, mulOp1PtrList, vectorizableBlock.iterationValueList);
        boolean isSamePtr2 = mulOp2PtrList.get(0) == mulOp2PtrList.get(1)
                && mulOp2PtrList.get(1) == mulOp2PtrList.get(2)
                && mulOp2PtrList.get(2) == mulOp2PtrList.get(3);
        boolean isCorrectPtr2 = checkPtr(vectorizableBlock.basicBlock, mulOp2PtrList, vectorizableBlock.iterationValueList);
        if (!isSamePtr1 && !isCorrectPtr1) {
            return false;
        }
        if (!isSamePtr2 && !isCorrectPtr2) {
            return false;
        }

//        System.out.println(vectorizableBlock.basicBlock);
        int regIdx1 = vectorizableBlock.basicBlock.getParentFunc().getAndAddRegIdx();
        int regIdx2 = vectorizableBlock.basicBlock.getParentFunc().getAndAddRegIdx();
        int regIdx3 = vectorizableBlock.basicBlock.getParentFunc().getAndAddRegIdx();
        Instruction vecMulOp1 = isSamePtr1 ?
                new VecDuplicateInstr(
                        mulOp1List.getFirst(), regIdx1, vectorizableBlock.basicBlock
                ) :
                new VecLoadInstr(
                        regIdx1, mulOp1PtrList.getFirst(), vectorizableBlock.basicBlock
                );
        Instruction vecMulOp2 = isSamePtr2 ?
                new VecDuplicateInstr(
                        mulOp2List.getFirst(), regIdx2, vectorizableBlock.basicBlock
                ) :
                new VecLoadInstr(
                        regIdx2, mulOp2PtrList.getFirst(), vectorizableBlock.basicBlock
                );
        Instruction vecValToAdd = new VecLoadInstr(
                regIdx3, pointerList.getFirst(), vectorizableBlock.basicBlock
        );
        vecMulOp1.insertBefore(vectorizableBlock.basicBlock.getLastInstr());
        vecMulOp2.insertBefore(vectorizableBlock.basicBlock.getLastInstr());
        vecValToAdd.insertBefore(vectorizableBlock.basicBlock.getLastInstr());

        int regIdx = vectorizableBlock.basicBlock.getParentFunc().getAndAddRegIdx();
        VecMulAddInstr vecMulAddInstr = new VecMulAddInstr(vecMulOp1, vecMulOp2, vecValToAdd, regIdx, vectorizableBlock.basicBlock);
        vecMulAddInstr.insertBefore(vectorizableBlock.basicBlock.getLastInstr());

        VecStoreInstr vecStore = new VecStoreInstr(
                vecMulAddInstr, vectorizableBlock.storePtrList.getFirst(), vectorizableBlock.basicBlock
        );
        vecStore.insertBefore(vectorizableBlock.basicBlock.getLastInstr());
        for (StoreInstr storeInstr : storeList) {
            storeInstr.removeFromList();
        }

        return true;
    }

    private static void mergeAdd(VectorizableBlock vectorizableBlock) {
        int regIdx = vectorizableBlock.basicBlock.getParentFunc().getAndAddRegIdx();
        AddInstr newAdd = new AddInstr(
                regIdx, vectorizableBlock.iterationValueList.getFirst(), new IntConst(4), vectorizableBlock.basicBlock
        );
        newAdd.insertAfter(vectorizableBlock.finalAdd);
        vectorizableBlock.finalAdd.replaceUseWith(newAdd);
    }

    private static void storeVectorization(VectorizableBlock vectorizableBlock) {
        List<StoreInstr> storeList = vectorizableBlock.storeList;
        VecMoveFromScaInstr vecMoveFromScaInstr = new VecMoveFromScaInstr(
                storeList.get(0).getValueToStore(), storeList.get(1).getValueToStore(),
                storeList.get(2).getValueToStore(), storeList.get(3).getValueToStore(),
                vectorizableBlock.basicBlock.getParentFunc().getAndAddRegIdx(), vectorizableBlock.basicBlock
        );
        VecStoreInstr vecStore = new VecStoreInstr(
                vecMoveFromScaInstr, vectorizableBlock.storePtrList.getFirst(), vectorizableBlock.basicBlock
        );
        for (StoreInstr storeInstr : storeList) {
            storeInstr.removeFromList();
        }
        vecMoveFromScaInstr.insertBefore(vectorizableBlock.basicBlock.getLastInstr());
        vecStore.insertBefore(vectorizableBlock.basicBlock.getLastInstr());
    }

    private static VectorizableBlock getVectorizableBlock(BasicBlock basicBlock) {
        List<Value> addList = null;

        Instruction ins = basicBlock.getLastInstr();
        while (ins != null) {
            if (addList == null
                    && ins instanceof AddInstr add4
                    && add4.getOperand2() instanceof IntConst intConst4 && intConst4.getConstInt() == 1
                    && add4.getOperand1() instanceof AddInstr add3
                    && add3.getOperand2() instanceof IntConst intConst3 && intConst3.getConstInt() == 1
                    && add3.getOperand1() instanceof AddInstr add2
                    && add2.getOperand2() instanceof IntConst intConst2 && intConst2.getConstInt() == 1
                    && add2.getOperand1() instanceof AddInstr add1
                    && add1.getOperand2() instanceof IntConst intConst1 && intConst1.getConstInt() == 1) {  // 四次累加 1
                addList = Arrays.asList(add1.getOperand1(), add1, add2, add3, add4);
            }
            ins = (Instruction) ins.getPrev();
        }

        List<GEPInstr> storePtrList = new ArrayList<>();
        List<StoreInstr> storeList = new ArrayList<>();
        for (Instruction instruction : basicBlock.getInstrList()) {
            if (instruction instanceof StoreInstr storeInstr && storeInstr.getPointer() instanceof GEPInstr storePtr) {
                storeList.add(storeInstr);
                storePtrList.add(storePtr);
            }
        }

        if (addList != null && storeList.size() == 4 && checkPtr(basicBlock, storePtrList, addList.subList(0, 4))) {
            return new VectorizableBlock(basicBlock, addList, storePtrList, storeList);
        }

        return null;
    }

    private static boolean checkPtr(BasicBlock curBlk, List<GEPInstr> ptrList, List<Value> itList) {
        if (ptrList.size() != 4 || itList.size() != 4) {
            return false;
        }
        for (int i = 0; i < 4; i++) {
            GEPInstr ptr = ptrList.get(i);

            if (((TPointer) ptr.getType()).getReferencedType() instanceof TFloat) {
                return false;   // todo 浮点运算向量化之后再说，因为需要一些特殊的后端指令才能应对
            }

            List<Value> indexList = ptr.getIndexList();
            for (int j = 0; j < indexList.size() - 1; j++) {
                Value index = indexList.get(j);
                if (index instanceof Instruction idxIns && idxIns.getParentBB() == curBlk) {
                    return false;
                }
            }
            if (indexList.getLast() != itList.get(i)) {
                return false;
            }
        }
        return true;
    }

    private static class VectorizableBlock {
        private final BasicBlock basicBlock;
        private final List<Value> iterationValueList;
        private final AddInstr finalAdd;
        private final List<GEPInstr> storePtrList;
        private final List<StoreInstr> storeList;

        public VectorizableBlock(BasicBlock basicBlock, List<Value> addList, List<GEPInstr> storePtrList, List<StoreInstr> storeList) {
            this.basicBlock = basicBlock;
            this.iterationValueList = addList.subList(0, 4);
            this.finalAdd = (AddInstr) addList.getLast();
            this.storePtrList = storePtrList;
            this.storeList = storeList;
        }

        @Override
        public String toString() {
            return "VEC " + this.basicBlock.toString();
        }
    }
}
