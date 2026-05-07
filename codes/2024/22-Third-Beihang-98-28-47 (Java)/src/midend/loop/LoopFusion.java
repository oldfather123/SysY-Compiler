package midend.loop;

import frontend.ir.Use;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstValue;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.binop.BinaryOperation;
import frontend.ir.instr.memop.GEPInstr;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.otherop.cmp.Cmp;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.structure.GlobalObject;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;

public class LoopFusion {
    public static void execute(ArrayList<Function> functions) {
        AnalysisLoop.execute(functions);
        for (Function function : functions) {
            // 要求在完全内联之后，只对 main 函数做循环展开，递归函数不做展开
            if (function.isMain()) {
                AnalysisLoop.LoopIndVars(function);
                for (Loop loop1 : function.getAllLoop()) {
                    for (Loop loop2 : function.getAllLoop()) {
                        if (canFusion(loop1, loop2)) {
                            fusion(loop1, loop2);
                        }
                    }
                }
            }
        }
    }

    private static void fusion(Loop loop1, Loop loop2) {
        if (false) {
            throw new RuntimeException("caole");
        }
        //anon: 将latch1的跳转改为到header2的跳转
        //anon: 将latch2的跳转改为到header1的跳转
        //anon: 融合body部分
        Value var1 = loop1.getVar();
        Value var2 = loop2.getVar();
        ArrayList<BasicBlock> moveBlks = new ArrayList<>();
        BasicBlock header2 = loop2.getHeader();
        BasicBlock header1 = loop1.getHeader();
        BasicBlock latch2 = loop2.getLatchs().get(0);
        BasicBlock latch1 = loop1.getLatchs().get(0);
        BasicBlock tmp = header2;
        BasicBlock last = latch1;
        latch2.getEndInstr().modifyUse(header2, header1);
        last.getEndInstr().removeFromList();
        last.setRet(false);
        last.addInstruction(new JumpInstr(header2));
        while (tmp != latch2) {
            tmp.removeFromListWithUseRemain();
            tmp.insertAfter(last);
            last = tmp;
            tmp = (BasicBlock) tmp.getNext();
        }
//        latch2.replaceUseTo(latch1);
//        tmp.getEndInstr().modifyUse(latch2, latch1);
        var2.replaceUseTo(var1);

        //anon: 将header2的phi放到header1
        BasicBlock preHeader1 = loop1.getPreHeader();
        BasicBlock preCond1 = null;
        for (BasicBlock block : preHeader1.getPres()) {
            preCond1 = block;
        }
        if (preCond1 == null) {
            return;
        }
        BasicBlock preHeader2 = loop2.getPreHeader();
        BasicBlock preCond2 = null;
        for (BasicBlock block : preHeader2.getPres()) {
            preCond2 = block;
        }
        if (preCond2 == null) {
            return;
        }
        Instruction headerPhi = header2.getEndInstr();
        Instruction headerPhiPos = (Instruction) header1.getInstructions().getHead();
        while (headerPhiPos instanceof PhiInstr) {
            ((PhiInstr) headerPhiPos).modifyPrtBlk(latch1, latch2);
            headerPhiPos = (Instruction) headerPhiPos.getNext();
        }
        while (!(headerPhi instanceof PhiInstr)) {
            headerPhi = (Instruction) headerPhi.getPrev();
        }
        Instruction tmpPhi;
        while (headerPhi instanceof PhiInstr) {
            tmpPhi = (Instruction) headerPhi.getPrev();
            headerPhi.removeFromListWithUseRemain();
            ((PhiInstr) headerPhi).modifyPrtBlk(preHeader2, preHeader1);
            headerPhi.insertBefore(headerPhiPos);
            headerPhiPos = headerPhi;
            headerPhi = tmpPhi;
        }

        //anon: 融合loopExit部分 由于removeUselessPhi, 所以这里只会在exit中存在phi，将exit1放到exit2中
        BasicBlock loopExit2 = loop2.getExits().get(0);
        BasicBlock loopExit1 = loop1.getExits().get(0);

        Instruction phiInloopExit1 = loopExit1.getEndInstr();
        Instruction lastPhi = (Instruction) loopExit2.getInstructions().getHead();
        Instruction tmpIns;

        while (phiInloopExit1 != null) {
            tmpIns = (Instruction) phiInloopExit1.getPrev();
            if (phiInloopExit1 instanceof PhiInstr) {
                phiInloopExit1.removeFromListWithUseRemain();
                phiInloopExit1.insertBefore(lastPhi);
                ((PhiInstr) phiInloopExit1).modifyPrtBlk(latch1, latch2);
                lastPhi = phiInloopExit1;
            }
            phiInloopExit1 = tmpIns;
        }



        BasicBlock exit1 = null;
        for (BasicBlock block : loopExit1.getSucs()) {
            exit1 = block;
        }
        if (exit1 == null) {
            return;
        }
        BasicBlock exit2 = null;
        for (BasicBlock block : loopExit2.getSucs()) {
            exit2 = block;
        }
        if (exit2 == null) {
            return;
        }

        loopExit1.replaceUseTo(loopExit2);
        Instruction phiPos = (Instruction) exit2.getInstructions().getHead();
        Instruction pos = phiPos;
        while (pos instanceof PhiInstr) {
            ((PhiInstr) pos).modifyPrtBlk(preCond2, preCond1);
            pos = (Instruction) pos.getNext();
        }

        Instruction instr = (Instruction) exit1.getEndInstr().getPrev();
        while (instr != null) {
            tmpIns = (Instruction) instr.getPrev();
            instr.removeFromListWithUseRemain();
            if (instr instanceof PhiInstr) {
                ((PhiInstr) instr).modifyPrtBlk(loopExit1, loopExit2);
                instr.insertBefore(phiPos);
                phiPos = instr;
            } else {
                instr.insertBefore(pos);
                pos = instr;
            }
            instr = tmpIns;
        }
        exit1.replaceUseTo(exit2);//anon: caole phi得自己修改
        //anon: 融合if部分, 原来会有上一次的exit跳到新的preCond =》把上一次的exit跳到新的exit之后




//        BasicBlock preHeader2 = loop2.getPreHeader();
//        BasicBlock preCond2 = null;
//        for (BasicBlock block : preHeader2.getPres()) {
//            preCond2 = block;
//        }
//        if (preCond2 == null) {
//            return;
//        }
//        exit1.getEndInstr().modifyUse(preHeader2, e);
    }

    private static boolean canFusion(Loop loop1, Loop loop2) {
        if (loop1 == loop2) {
            return false;
        }
        //最里层 or 单层？
        if (!loop1.getInnerLoops().isEmpty()) {
            return false;
        }
        if (!loop2.getInnerLoops().isEmpty()) {
            return false;
        }

        if (!loop1.hasIndVar()) {
            return false;
        }
        if (!loop2.hasIndVar()) {
            return false;
        }

        /*
        * anon：
        *  没有短路求值的情况
        *  所以一个循环的exit一定是另一个的precond
        * */

        BasicBlock preHeader1 = loop1.getPreHeader();
        BasicBlock preCond1 = null;
        if (preHeader1.getPres().size() == 1) {
            for (BasicBlock block : preHeader1.getPres()) {
                preCond1 = block;
            }
        }
        if (preCond1 == null) {
            return false;
        }
        BasicBlock loopExit1 = loop1.getExits().get(0);
        BasicBlock exit1 = null;
        if (loopExit1.getSucs().size() == 1) {
            for (BasicBlock block : loopExit1.getSucs()) {
                exit1 = block;
            }
        }
        if (exit1 == null) {
            return false;
        }

        BasicBlock preHeader2 = loop2.getPreHeader();
        BasicBlock preCond2 = null;
        if (preHeader2.getPres().size() == 1) {
            for (BasicBlock block : preHeader2.getPres()) {
                preCond2 = block;
            }
        }
        if (preCond2 == null) {
            return false;
        }
        BasicBlock loopExit2 = loop2.getExits().get(0);
        BasicBlock exit2 = null;
        if (loopExit2.getSucs().size() == 1) {
            for (BasicBlock block : loopExit2.getSucs()) {
                exit2 = block;
            }
        }
        if (exit2 == null) {
            return false;
        }
        //要有顺序，因为每对循环被判断了两次 1-》2
        if (!exit1.getSucs().contains(preCond2)) {
            return false;
        }

        //首尾相连了
        if(!loop1.getBegin().equals(loop2.getBegin())){
            return false;
        }
        if(!loop1.getEnd().equals(loop2.getEnd())){
            return false;
        }
        if(!loop1.getStep().equals(loop2.getStep())){
            return false;
        }
        int binCnt = 0;
        Value var1 = loop1.getVar();
        Value var2 = loop2.getVar();
        Use use = var1.getBeginUse();
        while (use != null) {
            Instruction user = use.getUser();
            if(user instanceof BinaryOperation){
                binCnt++;
            }
            use = (Use) use.getNext();
        }
        if (binCnt != 1) {
            return false;
        }

        binCnt = 0;
        use = var2.getBeginUse();
        while (use != null) {
            Instruction user = use.getUser();
            if(user instanceof BinaryOperation){
                binCnt++;
            }
            use = (Use) use.getNext();
        }
        if (binCnt != 1) {
            return false;
        }
//        StoreInstr store1 = null;
//        StoreInstr store2 = null;
        HashSet<LoadInstr>  loop1loading = new HashSet<>();
        HashSet<StoreInstr> loop2storing = new HashSet<>();
        HashSet<LoadInstr>  loop2loading = new HashSet<>();
        HashSet<StoreInstr> loop1storing = new HashSet<>();
        for (BasicBlock blk : loop1.getBlks()) {
            Instruction instruction = (Instruction) blk.getInstructions().getHead();
            while (instruction != null) {
                if (instruction instanceof StoreInstr) {
//                    if (store1 == null) {
//                        store1 = (StoreInstr) instruction;
//                    } else {
//                        return false;
//                    }
                    loop1storing.add((StoreInstr) instruction);
                } else if (instruction instanceof LoadInstr) {
                    loop1loading.add((LoadInstr) instruction);
                }
                instruction = (Instruction) instruction.getNext();
            }
        }
        for (BasicBlock blk : loop2.getBlks()) {
            Instruction instruction = (Instruction) blk.getInstructions().getHead();
            while (instruction != null) {
                if (instruction instanceof CallInstr) {
                    return false;
                } else if (instruction instanceof StoreInstr) {
//                    if (store2 == null) {
//                        store2 = (StoreInstr) instruction;
//                    } else {
//                        return false;
//                    }
                    loop2storing.add((StoreInstr) instruction);
                } else if (instruction instanceof LoadInstr) {
                    loop2loading.add((LoadInstr) instruction);
                }
                for (Value value : instruction.getUseValueList()) {
                    if (value instanceof PhiInstr && !checkPhiNotFromLoop((PhiInstr) value, loop1, new HashSet<>())) {
                       return false;
                    } else if (value instanceof Instruction && loop1.getBlks().contains(((Instruction) value).getParentBB())) {
                        return false;
                    }
                }

                instruction = (Instruction) instruction.getNext();
            }
        }
//        if (store1 != null && store2 != null && !store1.myHash().equals(store2.myHash())) { 这个store的比较就是错的
//            return false;
//        }
        
        for (LoadInstr load : loop1loading) {
            Value loadPtr = load.getPtr();
            if (loadPtr instanceof GEPInstr) {
                for (StoreInstr storeInstr : loop2storing) {
                    Value storePtr = storeInstr.getPtr();
                    if (storePtr instanceof GEPInstr) {
                        if (((GEPInstr) loadPtr).getRoot().equals(((GEPInstr) storePtr).getRoot())) {
                            return false;
                        }
                    }
                }
            } else if (loadPtr instanceof GlobalObject) {
                for (StoreInstr storeInstr : loop2storing) {
                    if (storeInstr.getPtr().equals(loadPtr)) {
                        return false;
                    }
                }
            } else {
                throw new RuntimeException("不应该有其它指针类型");
            }
        }
        
        // 这里我们认为，需要要求先存后取的地址必须是常数或者 phi（迭代变量）
        for (StoreInstr store : loop1storing) {
            if (checkNonSimpleIndex(store.getPtr())) {
                return false;
            }
        }
        for (LoadInstr load : loop2loading) {
            if (checkNonSimpleIndex(load.getPtr())) {
                return false;
            }
        }

        return true;
    }
    
    private static boolean checkNonSimpleIndex(Value ptr) {
        if (ptr instanceof GEPInstr) {
            List<Value> indexList = ((GEPInstr) ptr).getWholeIndexList();
            for (Value index : indexList) {
                if (!(index instanceof ConstValue) && !(index instanceof PhiInstr)) {
                    return true;
                }
            }
        }
        return false;
    }
    
    private static boolean checkPhiNotFromLoop(PhiInstr init, Loop loop, HashSet<PhiInstr> done) {
        if (done.contains(init)) {
            return false;
        }
        done.add(init);
        for (Value val : init.getValues()) {
            if (val instanceof ConstValue) {
                continue;
            }
            if (val instanceof PhiInstr) {
                if (checkPhiNotFromLoop((PhiInstr) val, loop, done)) {
                    return true;
                }
                continue;
            }
            if (val instanceof Instruction) {
                if (!loop.getBlks().contains(((Instruction) val).getParentBB())) {
                    return true;
                }
            } else {
                throw new RuntimeException("这里不应该出现其它 value");
            }
        }
        return false;
    }
}
