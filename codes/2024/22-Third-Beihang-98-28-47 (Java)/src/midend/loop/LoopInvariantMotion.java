package midend.loop;

import frontend.ir.Use;
import frontend.ir.Value;
import frontend.ir.constvalue.ConstValue;
import frontend.ir.instr.Instruction;
import frontend.ir.instr.memop.LoadInstr;
import frontend.ir.instr.memop.StoreInstr;
import frontend.ir.instr.otherop.CallInstr;
import frontend.ir.instr.otherop.PhiInstr;
import frontend.ir.instr.terminator.JumpInstr;
import frontend.ir.instr.terminator.Terminator;
import frontend.ir.structure.BasicBlock;
import frontend.ir.structure.Function;
import frontend.ir.structure.Procedure;

import java.util.*;

public class LoopInvariantMotion {
    public static void execute(ArrayList<Function> functions) {
        for (Function function : functions) {
            findInvariant(function);
        }
    }

    private static void findInvariant(Function function) {
        for (Loop loop : function.getOuterLoop()) {
            findInvar4loop(loop);
        }
    }
    /*
    * anon:
    *  不被使用的量后提
    *  若有多个exiting块则不能处理, 因为不知道循环的结束条件?
    *  要找到结束的值, 进行替换
    *  只被定义不被使用
    *
    * */

    private static void findInvar4loop(Loop loop) {
        for (Loop inner : loop.getInnerLoops()) {
            findInvar4loop(inner);
        }
//        loop.LoopPrint();
        //System.out.println(loop.getHeader() + " entering " + loop.getEntering() + " " + loop.getSameLoopDepth());
        Queue<Instruction> queue = new LinkedList<>();
        HashSet<Value> invariant = new HashSet<>();
        //不需要使用dom，由于是ssa，所以可以直接硬提 // 会导致时间变慢
        AnalysisLoop.dom4loop(loop);
//        for (BasicBlock blk : loop.getBlks()) {
//            System.out.println(blk + " loopDoms " + blk.getLoopDoms());
//        }
        for (BasicBlock block : loop.getSameLoopDepth()) {
            Instruction instr = (Instruction) block.getInstructions().getHead();
            while (instr != null) {
                if (defOutOfLoop(instr, loop, invariant)) {
                    queue.add(instr);
                    invariant.add(instr);
                }
                instr = (Instruction) instr.getNext();
            }
        }
        //TODO: if-do-while 各个blk的归属
        BasicBlock preHeader = loop.getPreHeader();
        int cnt = 0;
        Instruction last = null;
        while (!queue.isEmpty()) {
            Instruction instr = queue.poll();
            instr.removeFromListWithUseRemain();
            if (cnt == 0) {
                preHeader.addInstrToHead(instr);
            } else {
                instr.insertAfter(last);
            }
            Use use = instr.getBeginUse();
            while (use != null) {
                Instruction user = use.getUser();
                if (defOutOfLoop(user, loop, invariant)) {
                    if (!invariant.contains(user)) {
                        invariant.add(user);
                        queue.add(user);
                    }
                }
                use = (Use) use.getNext();
            }
            cnt++;
            last = instr;
        }

//        for (BasicBlock block : loop.getSameLoopDepth()) {
//            Instruction instr = (Instruction) block.getInstructions().getHead();
//            while (instr != null) {
//                if (defOutOfLoop(instr, loop, invariant)) {
//                    queue.add(instr);
//                    invariant.add(instr);
//                }
//                instr = (Instruction) instr.getNext();
//            }
//        }
    }

    private static void addTmpBlk(BasicBlock entering, BasicBlock tmpBlk, BasicBlock next) {
        entering.getEndInstr().modifyUse(next, tmpBlk);
        tmpBlk.addInstruction(new JumpInstr(next));
        Instruction instr = (Instruction) next.getInstructions().getHead();
        while (instr instanceof PhiInstr) {
            ((PhiInstr) instr).modifyPrtBlk(entering, tmpBlk);
            instr = (Instruction) instr.getNext();
        }
        tmpBlk.insertAfter(entering);
    }

    //instr所使用的值是否都来自循环外
    public static boolean defOutOfLoop(Instruction instr, Loop loop, HashSet<Value> invariant) {
        if (!loop.getBlks().contains(instr.getParentBB())) {
            return false;
        }
//        if (instr instanceof LoadInstr || instr instanceof StoreInstr) {
//            return false;
//        }
        if (instr instanceof LoadInstr && ((LoadInstr) instr).mayBeStored(loop.getBlks())) {
            return false;
        }
        if (instr instanceof StoreInstr/* && ((StoreInstr) instr).mayBeLoaded(loop.getBlks())*/) {
            // todo: store 不能简单提取，如果在分支里则不能外提，之后可以通过更强力的数组 SSA 或者对循环内部支配关系的解析来解决这个问题
            return false;
        }
        if (instr instanceof CallInstr) {
            //no side effect should be lift;
            if (!((CallInstr) instr).checkNoSideEffect()) {
                return false;
            }
        }
        if (instr instanceof Terminator) {
            return false;
        }
        if (instr instanceof PhiInstr) {
            return false;
        }
        if (!instr.getParentBB().getLoopDoms().containsAll(loop.getExits())) {
            return false;
        }
        for (Value value : instr.getUseValueList()) {
            if (value instanceof ConstValue) {
                continue;
            } else if (invariant.contains(value)) {
                continue;
            } else if (value instanceof Instruction && !loop.getBlks().contains(((Instruction) value).getParentBB())) {
                continue;
            }
            return false;
        }
        return true;
    }
}
